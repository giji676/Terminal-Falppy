#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <time.h>
#include <string.h>
#include <math.h>

struct termios oldt, newt;

double get_time_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int kbhit() {
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

void draw_death_box(struct winsize w, double t) {
    int box_w = 30;
    int box_h = 7;
    int start_row = (w.ws_row - box_h) / 2;
    int start_col = (w.ws_col - box_w) / 2;

    // Pulsing color (brightness changes over time)
    // int intensity = (int)(fabs(sin(t * 2)) * 3) + 1; // 1–4
    // int color = 41 + intensity; // 41–45 are red–magenta shades
    //
    // for (int r = 0; r < box_h; r++) {
    //     printf("\e[%d;%dH", start_row + r, start_col);
    //     for (int c = 0; c < box_w; c++) {
    //         if (r == 0 || r == box_h - 1 || c == 0 || c == box_w - 1)
    //             printf("\e[%dm \e[0m", color);
    //         else
    //             printf(" ");
    //     }
    // }

    const char *msg1 = "!!  YOU DIED  !!";
    const char *msg2 = "Press Q to Quit";
    printf("\e[%d;%dH%s", start_row + box_h / 2 - 1,
           start_col + (box_w - (int)strlen(msg1)) / 2, msg1);
    printf("\e[%d;%dH%s", start_row + box_h / 2 + 1,
           start_col + (box_w - (int)strlen(msg2)) / 2, msg2);
    fflush(stdout);
}

void reset_terminal() {
    printf("\e[m"); // reset color changes
    printf("\e[?25h"); // show cursor
    printf("\e[2J\e[H"); // clear terminal
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
    atexit(reset_terminal);
}

int main() {
    configure_terminal();

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    char c;
    int running = 1, paused = 0, dead = 0;

    const char *bird[] = {
        "\e[0;33m /==\e[0;37m@\e[0;33m\\\e[0m\0",
        "\e[0;33m<===\e[0;37m@@\e[0;33m=\e[0;31m=>\e[0m\0",
        "\e[0;33m \\===/\e[0m\0"
    };
    int bird_rows = sizeof(bird) / sizeof(bird[0]);

    int x = w.ws_col * 0.1;
    double y = w.ws_row / 2.0;
    double v = 0, g = 30.0, flap_strength = -15.0;

    double prev_time = get_time_seconds();
    double death_time = 0.0;

    int pipe_w = 10;
    int pipe_h = w.ws_row;
    char pipe[pipe_h][pipe_w+1];

    for (int i = 0; i < pipe_h; i++) {
        for (int j = 0; j <= pipe_w; j++) {
            if (j == pipe_w) { pipe[i][j] = '\0'; }
            else { pipe[i][j] = '#'; }
        }
    }

    int pipe_x = w.ws_col-pipe_w;
    int pipe_y = 1;

    int debug_y = w.ws_row - 5;
    int debug_x = 2;

    while (running) {
        double now = get_time_seconds();
        double dt = now - prev_time;
        prev_time = now;

        if (!dead) {
            // Input
            if (kbhit()) {
                read(STDIN_FILENO, &c, 1);
                if (c == 'q') running = 0;
                if (c == ' ') v = flap_strength;
                if (c == 'p') paused = paused ? 0 : 1;
            }

            if (!paused) {
                // Physics
                v += g * dt;
                y += v * dt;

                // Pipe movement
                pipe_x -= 40 * dt;
            }

            // Clear screen
            printf("\e[2J\e[H");

            // Render pipe
            for (int i = 0; i < pipe_h; i++) {
                printf("\e[0;32m\e[%d;%dH%s\e[0m", pipe_y+i, pipe_x, pipe[i]);
            }

            // Render bird
            for (int i = 0; i < bird_rows; i++) {
                printf("\e[%d;%dH%s", (int)y + i, x, bird[i]);
            }

            // Debug
            printf("\e[%d;%dHpipe_x: %d", debug_y, debug_x, pipe_x);
            printf("\e[%d;%dHbird_y: %.2f", debug_y + 1, debug_x, y);
            printf("\e[%d;%dHdt: %.4f", debug_y + 2, debug_x, dt);
            printf("\e[%d;%dHv: %.2f", debug_y + 3, debug_x, v);

            // Collision
            if (y > w.ws_row - bird_rows || y < 1) {
                dead = 1;
                v = 0;
                death_time = now;
            }
            fflush(stdout);
            usleep(16000);
        } else {
            // Draw static scene (objects frozen)
            // Render bird
            printf("\e[2J\e[H");
            for (int i = 0; i < bird_rows; i++) {
                printf("\e[%d;%dH%s", (int)y + i, x, bird[i]);
            }

            // Render pipe
            for (int i = 0; i < pipe_h; i++) {
                printf("\e[0;32m\e[%d;%dH%s\e[0m", pipe_y+i, pipe_x, pipe[i]);
            }

            // Debug
            printf("\e[%d;%dHpipe_x: %d", debug_y, debug_x, pipe_x);
            printf("\e[%d;%dHbird_y: %.2f", debug_y + 1, debug_x, y);
            printf("\e[%d;%dHdt: %.4f", debug_y + 2, debug_x, dt);
            printf("\e[%d;%dHv: %.2f", debug_y + 3, debug_x, v);


            // Overlay pulsing death box
            draw_death_box(w, now - death_time);

            if (kbhit()) {
                read(STDIN_FILENO, &c, 1);
                if (c == 'q' || c == 'Q') running = 0;
            }

            usleep(16000);
        }
    }

    return 0;
}
