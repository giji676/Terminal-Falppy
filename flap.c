#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

struct termios oldt, newt;

typedef struct {
    int width;
    int height;
} Bird;

const char *bird[] = {
    "\e[0;33m /==\e[0;37m@\e[0;33m\\\e[0m\0",
    "\e[0;33m<===\e[0;37m@@\e[0;33m=\e[0;31m=>\e[0m\0",
    "\e[0;33m \\===/\e[0m\0"
};

typedef struct {
    int width;
    int height;
    char *screen;
    char *old_screen;
} GameState;

void render(GameState *state) {
    for (int i = 0; i < state->height; i++) {
        for (int j = 0; j < state->width; j++) {
            printf("\e[%d;%dH%c", i + 1, j + 1, state->screen[i * state->width + j]);
        }
    }
    fflush(stdout);
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

int main() {
    srand(time(NULL));

    configure_terminal();

    GameState game_state;
    initialize_screen(&game_state);

    int x;
    while (1) {
        clear_state_screen(&game_state);
        x = rand() % (game_state.width * game_state.height);
        game_state.screen[x] = 'x';
        render(&game_state);
        // Copy the screen to old_screen
        memcpy(game_state.old_screen, game_state.screen, game_state.width * game_state.height);
        usleep(10000);
    }

    cleanup_state(&game_state);

    return 0;
}
