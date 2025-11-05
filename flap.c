#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#define MAX_PIPES 5 
#define PIPES_WIDTH 10
#define PIPES_MIN_V_GAP 10 // Vertical gap
#define PIPES_MAX_V_GAP 15 // Vertical gap

struct termios oldt, newt;
int width, height;

typedef struct {
    int width, height;
    int x;
    double y;
    const char **lines;
} Bird;

typedef struct {
    double x;
    int t_h; // Top pipe height
    int b_y; // Bottom pipe y
} Pipe;

const char *bird_lines[] = {
    "\e[0;33m /==\e[0;37m@\e[0;33m\\\e[0m\0",
    "\e[0;33m<===\e[0;37m@@\e[0;33m=\e[0;31m=>\e[0m\0",
    "\e[0;33m \\===/\e[0m\0"
};

int kbhit() {
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}


void render(Bird *bird, Pipe pipes[]) {
    printf("\e[2J\e[H"); // clear terminal

    for (int i = 0; i < MAX_PIPES; i++) {
        for (int row = 0; row < pipes[i].t_h; row++) {
            for (int sx = 0; sx < PIPES_WIDTH; sx++) {
                int col = (int)round(pipes[i].x) + sx;
                if (col >= 0 && col < width && row >= 0 && row < height) {
                    printf("\e[%d;%dH\e[32m#", row + 1, col + 1);
                }
            }
        }

        for (int row = pipes[i].b_y; row < height; row++) {
            for (int sx = 0; sx < PIPES_WIDTH; sx++) {
                int col = (int)round(pipes[i].x) + sx;
                if (col >= 0 && col < width && row >= 0 && row < height) {
                    printf("\e[%d;%dH#", row + 1, col + 1);
                }
            }
        }
    }

    for (int i = 0; i < bird->height; i++) {
        int row = bird->y + i;
        int col = bird->x;
        if (row >= 0 && row < height && col >= 0 && col < width) {
            printf("\e[%d;%dH%s", row + 1, col + 1, bird->lines[i]);
        }
    }
    fflush(stdout);
}

int random_in_range(int min, int max) {
    if (min > max) {
        int tmp = min;
        min = max;
        max = tmp;
    }
    return rand() % (max - min + 1) + min;
}

void initialize_pipes(Pipe *pipes, double gap) {
    for (int i = 0; i < MAX_PIPES; i++) {
        pipes[i].x = (double)width + gap * i;
        // Random vertical gap distance
        int v_gap = random_in_range(PIPES_MIN_V_GAP, PIPES_MAX_V_GAP);
        // Random height of top pipe
        pipes[i].t_h = random_in_range((int)(height * 0.3), (int)(height * 0.7));
        pipes[i].b_y = pipes[i].t_h + v_gap;
    }
}

void initialize_bird(Bird *bird) {
    bird->lines = bird_lines;
    bird->height = sizeof(bird_lines) / sizeof(bird_lines[0]);

    // Calculate max visible width (ignoring ANSI codes)
    bird->width = 0;
    for (int i = 0; i < bird->height; i++) {
        int len = 0;
        const char *p = bird->lines[i];
        while (*p) {
            if (*p == '\e') { // skip ANSI sequences
                while (*p && *p != 'm') p++;
                if (*p) p++;
            } else if (*p == '\\' && *(p+1) == '\\') { // escaped backslash
                len++; // counts as 1 visible char
                p += 2; // skip both chars in string
            } else {
                len++;
                p++;
            }
        }
        if (len > bird->width) bird->width = len;
    }
    bird->x = (int)(width * 0.1);
    bird->y = (int)(height * 0.5 - bird->height / 2);
}

void reset_terminal() {
    printf("\e[m"); // reset color changes
    printf("\e[?25h"); // show cursor
    printf("\e[2J\e[H"); // clear terminal
    printf("\e[4h"); // Enable insert mode
    printf("\e[?7h");  // re-enable when done
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

void configure_terminal() {
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    printf("\e[?25l"); // hide cursor
    printf("\e[2J\e[H"); // clear terminal
    printf("\e[4l"); // Disable insert mode
    printf("\e[?7l");  // disable auto-wrap
    atexit(reset_terminal);
}

void handle_sigint(int sig) {
    reset_terminal();
    _exit(0);
}

double get_time_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void update_pipes(Pipe *pipes, double speed, double gap, double dt) {
    double farthest_x = pipes[0].x;
    for (int i = 1; i < MAX_PIPES; i++) {
        if (pipes[i].x > farthest_x) farthest_x = pipes[i].x;
    }

    for (int i = 0; i < MAX_PIPES; i++) {
        pipes[i].x -= dt * speed;  // move left

        /* When completely off the left side (right edge < 0), wrap it */
        if (pipes[i].x + PIPES_WIDTH < 0) {
            pipes[i].x = farthest_x + gap;
            farthest_x = pipes[i].x; /* update farthest so multiple wraps stack correctly */
            /* optionally randomize vertical gap/height again on wrap: */
            int v_gap = random_in_range(PIPES_MIN_V_GAP, PIPES_MAX_V_GAP);
            pipes[i].t_h = random_in_range((int)(height * 0.3), (int)(height * 0.7));
            pipes[i].b_y = pipes[i].t_h + v_gap;
        }
    }
}

int check_collision(Bird *bird, Pipe *pipes) {
    for (int i = 0; i < MAX_PIPES; i++) {
        if (pipes[i].x > bird->x + bird->width) {
            continue;
        }
        if (pipes[i].x + PIPES_WIDTH < bird->x) {
            continue;
        }
        if (bird->y < pipes[i].t_h) {
            return 1;
        }
        if (bird->y + bird->height > pipes[i].b_y) {
            return 1;
        }
    }
    return 0;
}

int check_death(Bird *bird) {
    if (bird->y - bird->height > height || bird->y < 1) {
        return 1;
    }
    return 0;
}

void death_screen() {
    const char *msg0 = "!! YOU  DIED !!";
    const char *msg1 = "Press Q to Quit";

    int len0 = strlen(msg0);
    int len1 = strlen(msg1);

    // Center on screen
    int y0 = height / 2;
    int y1 = y0 + 1;
    int x0 = width / 2 - len0 / 2;
    int x1 = width / 2 - len1 / 2;

    // Clear screen & print centered text
    printf("\e[2J");
    printf("\e[%d;%dH%s", y0, x0, msg0);
    printf("\e[%d;%dH%s", y1, x1, msg1);
    fflush(stdout);
}

int main() {
    srand(time(NULL));
    configure_terminal();
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    width = w.ws_col;
    height = w.ws_row;

    Bird bird;
    initialize_bird(&bird);

    int pipes_gap = 80;
    double pipes_speed = 40;

    Pipe pipes[MAX_PIPES];
    initialize_pipes(pipes, pipes_gap);

    signal(SIGINT, handle_sigint);
    double prev_time = get_time_seconds();

    char c;

    double v = 0.0;
    double g = 20;
    double jump_f = -12;

    int running = 1;
    int paused = 0;
    int is_dead = 0;
    while (running) {
        if (!is_dead) {
            // Get delta-time
            double now = get_time_seconds();
            double dt = now - prev_time;
            prev_time = now;

            if (kbhit()) {
                read(STDIN_FILENO, &c, 1);
                switch (c) {
                    case 'q':
                    case 'Q':
                        running = 0;
                        break;
                    case 'p':
                    case 'P':
                        paused = paused ? 0 : 1;
                        break;
                    case ' ':
                        v = jump_f;
                        break;
                }
            }

            if (!paused) {
                v += g * dt;
                bird.y += v * dt;
                pipes_speed += dt * 0.9;
                update_pipes(pipes, pipes_speed, pipes_gap, dt);
            }

            render(&bird, pipes);
            is_dead = check_death(&bird);
            is_dead = check_collision(&bird, pipes);
            usleep(10000);
        } else {
            death_screen();
            if (kbhit()) {
                read(STDIN_FILENO, &c, 1);
                if (c == 'q' || c == 'Q') running = 0;

                switch (c) {
                    case 'q':
                    case 'Q':
                        running = 0;
                        break;
                }
            }
            usleep(10000);
        }
    }

    return 0;
}
