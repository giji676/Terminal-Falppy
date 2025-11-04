#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

struct termios oldt, newt;

typedef struct {
    int width, height;
    int x, y;
    const char **lines;
} Bird;

// const char *bird_lines[] = {
//     "\e[0;33m /==\e[0;37m@\e[0;33m\\\e[0m\0",
//     "\e[0;33m<===\e[0;37m@@\e[0;33m=\e[0;31m=>\e[0m\0",
//     "\e[0;33m \\===/\e[0m\0"
// };
//
const char *bird_lines[] = {
    " /==@\\",
    "<===@@==>",
    " \\===/"
};


typedef struct {
    int width, height;
    char *screen, *old_screen;
} GameState;

void render(GameState *state) {
    for (int i = 0; i < state->height; i++) {
        for (int j = 0; j < state->width; j++) {
            char c = state->screen[i * state->width + j];
            printf("\e[%d;%dH%c", i + 1, j + 1, c);
        }
    }
    fflush(stdout);
}

void update_screen(GameState *state, Bird *bird) {
    for (int i = 0; i < bird->height; i++) {
        int row = bird->y + i;
        if (row < 0 || row >= state->height) continue;

        const char *line = bird->lines[i];
        for (int j = 0; line[j] != '\0'; j++) {  // stop at null terminator
            int col = bird->x + j;
            if (col < 0 || col >= state->width) continue;
            state->screen[row * state->width + col] = line[j];
        }
    }
}

void initialize_screen(GameState *state) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    state->width = w.ws_col;
    state->height= w.ws_row;

    size_t size = state->width * state->height;

    state->screen = malloc(size);
    state->old_screen = malloc(size);

    memset(state->screen, ' ', size);
    memset(state->old_screen, ' ', size);
}

void initialize_bird(Bird *bird, GameState *state) {
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
                len++;   // counts as 1 visible char
                p += 2;  // skip both chars in string
            } else {
                len++;
                p++;
            }
        }
        if (len > bird->width) bird->width = len;
    }
    bird->x = (int)(state->width * 0.1);
    bird->y = (int)(state->height * 0.5 - bird->height / 2);
}

void clear_state_screen(GameState *state) {
    size_t size = state->width * state->height;
    memset(state->screen, ' ', size);
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

void cleanup_state(GameState *state) {
    free(state->screen);
    free(state->old_screen);
}

GameState *global_state;

void handle_sigint(int sig) {
    if (global_state) cleanup_state(global_state);
    reset_terminal();
    _exit(0);
}

int main() {
    srand(time(NULL));
    configure_terminal();

    GameState game_state;
    global_state = &game_state;

    Bird bird;
    initialize_screen(&game_state);
    initialize_bird(&bird, &game_state);

    signal(SIGINT, handle_sigint);
    while (1) {
        clear_state_screen(&game_state);
        update_screen(&game_state, &bird);
        render(&game_state);
        memcpy(game_state.old_screen, game_state.screen, game_state.width * game_state.height);
        usleep(10000);
    }

    cleanup_state(&game_state);

    return 0;
}
