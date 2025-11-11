#include <stdint.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define BLOCK_MULT_X 2 
#define NUM_SHAPES 7

struct termios oldt, newt;
int width, height;

typedef struct {
    int width, height;
    uint8_t *shape;
} Shape;

typedef struct {
    Shape *src_type;
    Shape *type;
    int x, y;
} ActivePiece;

ActivePiece *g_piece = NULL;
Shape *shapes[NUM_SHAPES];

typedef struct {
    int board[BOARD_HEIGHT][BOARD_WIDTH];
    Shape hold_shape;
    ActivePiece activePiece;
    int total_lines;
    int score;
    int level;
    int running;
    int hold_used;
    int game_over;
} GameState;

uint8_t shape_s[2*3] = {
    0,1,1,
    1,1,0
};

uint8_t  shape_z[2*3] = {
    1,1,0,
    0,1,1
};

uint8_t shape_t[2*3] = {
    0,1,0,
    1,1,1
};

uint8_t shape_l[3*2] = {
    1,0,
    1,0,
    1,1,
};

uint8_t shape_j[3*2] = {
    0,1,
    0,1,
    1,1,
};

uint8_t shape_o[2*2] = {
    1,1,
    1,1
};

uint8_t shape_i[4*1] = {
    1,
    1,
    1,
    1
};

int kbhit() {
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
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

double get_time_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void initialize_shapes(Shape *shapes[]) {
    shapes[0] = malloc(sizeof(Shape));
    shapes[0]->shape = shape_s;
    shapes[0]->width = 3;
    shapes[0]->height = 2;

    shapes[1] = malloc(sizeof(Shape));
    shapes[1]->shape = shape_z;
    shapes[1]->width = 3;
    shapes[1]->height = 2;

    shapes[2] = malloc(sizeof(Shape));
    shapes[2]->shape = shape_t;
    shapes[2]->width = 3;
    shapes[2]->height = 2;

    shapes[3] = malloc(sizeof(Shape));
    shapes[3]->shape = shape_o;
    shapes[3]->width = 2;
    shapes[3]->height = 2;

    shapes[4] = malloc(sizeof(Shape));
    shapes[4]->shape = shape_i;
    shapes[4]->width = 1;
    shapes[4]->height = 4;

    shapes[5] = malloc(sizeof(Shape));
    shapes[5]->shape = shape_j;
    shapes[5]->width = 2;
    shapes[5]->height = 3;

    shapes[6] = malloc(sizeof(Shape));
    shapes[6]->shape = shape_l;
    shapes[6]->width = 2;
    shapes[6]->height = 3;
}

void spawn_piece(GameState *state) {
    Shape *src = shapes[rand() % NUM_SHAPES];
    int size = src->width * src->height;
    ActivePiece *piece = &state->activePiece;

    if (piece->type == NULL) {
        piece->type = malloc(sizeof(Shape));
        piece->type->shape = malloc(sizeof(uint8_t) * size);
    } else if (piece->type->shape == NULL) {
        piece->type->shape = malloc(sizeof(uint8_t) * size);
    } else {
        // if existing buffer too small, realloc
        piece->type->shape = realloc(piece->type->shape, sizeof(uint8_t) * size);
    }

    memcpy(piece->type->shape, src->shape, sizeof(uint8_t) * size);
    piece->type->width = src->width;
    piece->type->height = src->height;

    piece->src_type = src;
    piece->x = BOARD_WIDTH / 2 - src->width / 2;
    piece->y = 0;
    for (int i = 0; i < piece->type->height; i++) {
        for (int j = 0; j < piece->type->height; j++) {
            if (piece->type->shape[i * piece->type->width + j] &&
                state->board[piece->y + i][piece->x + j]) {
                state->game_over = 1;
            }
        }
    }
    state->hold_used = 0;
}

void render(GameState *state) {
    int y_offset = (height / 2) - (BOARD_HEIGHT * 0.5);
    int x_offset = (width / 2) - (BOARD_WIDTH * BLOCK_MULT_X * 0.5);

    for (int i = 0; i < BOARD_HEIGHT + 2; i++) {
        if (i <= BOARD_HEIGHT) {
            printf("\e[%d;%dH\e[32m%s",
                   (i + 1) + y_offset, 
                   x_offset,
                   "<!");
            printf("\e[%d;%dH\e[32m%s",
                   (i + 1) + y_offset, 
                   x_offset + BOARD_WIDTH * BLOCK_MULT_X + 2,
                   "!>");
        }
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (i == BOARD_HEIGHT) {
                printf("\e[%d;%dH\e[32m%s",
                       (i + 1) + y_offset, 
                       (j + 1) * BLOCK_MULT_X + x_offset, 
                       "==");
            } else if (i == BOARD_HEIGHT + 1) {
                printf("\e[%d;%dH\e[32m%s",
                       (i + 1) + y_offset, 
                       (j + 1) * BLOCK_MULT_X + x_offset, 
                       "\\/");
            } else {
                if (state->board[i][j]) {
                    printf("\e[%d;%dH\e[32m%s",
                           (i + 1) + y_offset, 
                           (j + 1) * BLOCK_MULT_X + x_offset, 
                           "[]");
                } else {
                    printf("\e[%d;%dH\e[32m%s",
                           (i + 1) + y_offset, 
                           (j + 1) * BLOCK_MULT_X + x_offset, 
                           " .");
                }
            }
        }
    }

    ActivePiece *piece = &state->activePiece;
    Shape *shape = piece->type;

    for (int y = 0; y < shape->height; y++) {
        for (int x = 0; x < shape->width; x++) {
            if (shape->shape[y * shape->width + x]) {
                printf("\e[%d;%dH\e[32m%s",
                       (piece->y + 1 + y) + y_offset,
                       (piece->x + 1 + x) * BLOCK_MULT_X + x_offset,
                       "[]");
            }
        }
    }
}

void free_shapes() {
    for (int i = 0; i < NUM_SHAPES; i++) {
        free(shapes[i]);
    }
}

void handle_sigint(int sig) {
    if (g_piece && g_piece->type) {
        free(g_piece->type->shape);
        free(g_piece->type);
        g_piece->type = NULL;
    }
    free_shapes();
    reset_terminal();
    exit(0);
}

int check_fall(GameState *state) {
    ActivePiece *piece = &state->activePiece;
    Shape *shape = piece->type;

    for (int y = 0; y < shape->height; y++) {
        for (int x = 0; x < shape->width; x++) {
            if (piece->y + y == BOARD_HEIGHT - 1) {
                return 0;
            }
            if (!shape->shape[y * shape->width + x]) {
                continue;
            }
            if (state->board[piece->y + y + 1][piece->x + x]) {
                return 0;
            }
        }
    }
    return 1;
}

void hard_drop(GameState *state) {
    while (check_fall(state)) {
        state->activePiece.y++;
    }
}

void try_move(GameState *state, int dir) {
    ActivePiece *piece = &state->activePiece;
    Shape *shape = piece->type;

    for (int y = 0; y < shape->height; y++) {
        for (int x = 0; x < shape->width; x++) {
            if (dir == 1) {
                if (piece->x + x == BOARD_WIDTH - 1) {
                    return;
                }
            } else {
                if (piece->x == 0) {
                    return;
                }
            }
            if (!shape->shape[y * shape->width + x]) {
                continue;
            }
            if (state->board[piece->y + y][piece->x + x + dir]) {
                return;
            }
        }
    }
    piece->x += dir;
}

void update_state(GameState *state) {
    ActivePiece *piece = &state->activePiece;
    Shape *shape = piece->type;
    for (int y = 0; y < shape->height; y++) {
        for (int x = 0; x < shape->width; x++) {
            if (shape->shape[y * shape->width + x]) {
                state->board[piece->y+y][piece->x+x] = 1;
            }
        }
    }
}

void debug(GameState *state) {
    ActivePiece *piece = &state->activePiece;
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            printf("\e[%d;%dH%i", i, j, state->board[i][j]);
        }
    }
    Shape *shape = piece->type;
    for (int y = 0; y < shape->height; y++) {
        for (int x = 0; x < shape->width; x++) {
            uint8_t val = shape->shape[y * shape->width + x] * 2;
            if (val != 0) {
                printf("\e[%d;%dH%u",
                       piece->y + y,
                       piece->x + x,
                       val);
            }
        }
    }
    printf("\e[%d;%dH(%i,%i)", BOARD_HEIGHT+1, 1, piece->y, piece->x);
}

int check_clear(GameState *state) {
    int cleared = 0;
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        int full = 1;
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (!state->board[i][j]) {
                full = 0;
                break;
            }
        }
        if (full) {
            for (int k = i; k > 0; k--) {
                for (int j = 0; j < BOARD_WIDTH; j++) {
                    state->board[k][j] = state->board[k - 1][j];
                }
            }
            for (int j = 0; j < BOARD_WIDTH; j++) {
                state->board[0][j] = 0;
            }
            i--;
            cleared++;
        }
    }
    return cleared;
}

void rotate_shape(GameState *state) {
    ActivePiece *piece = &state->activePiece;
    Shape *shape = piece->type;
    int new_w = shape->height;
    int new_h = shape->width;

    uint8_t rotated[new_h * new_w];

    for (int y = 0; y < shape->height; y++) {
        for (int x = 0; x < shape->width; x++) {
            rotated[x * new_w + (new_w - 1 - y)] = shape->shape[y * shape->width + x];
        }
    }

    for (int y = 0; y < new_h; y++) {
        for (int x = 0; x < new_w; x++) {
            if (!rotated[y * new_w + x])
                continue;

            int new_x = piece->x + x;
            int new_y = piece->y + y;

            if (new_x < 0 || new_x >= BOARD_WIDTH || new_y < 0 || new_y >= BOARD_HEIGHT)
                return;

            if (state->board[new_y][new_x])
                return;
        }
    }

    for (int i = 0; i < shape->width * shape->height; i++)
        shape->shape[i] = rotated[i];

    shape->width = new_w;
    shape->height = new_h;
}

void swap_shape(GameState *state) {
    ActivePiece *piece = &state->activePiece;
    Shape *shape = &state->hold_shape;
    Shape *temp_src = piece->src_type;
    piece->src_type = shape;

    free(piece->type->shape);
    piece->type->width = shape->width;
    piece->type->height = shape->height;
    int size = shape->width * shape->height;
    piece->type->shape = malloc(sizeof(uint8_t) * size);
    memcpy(piece->type->shape, shape->shape, sizeof(uint8_t) * size);

    piece->x = BOARD_WIDTH / 2 - piece->type->width / 2;
    piece->y = 0;

    shape->shape = temp_src->shape;
    shape->width = temp_src->width;
    shape->height = temp_src->height;
}

void hold(GameState *state) {
    ActivePiece *piece = &state->activePiece;
    Shape *hold_shape = &state->hold_shape;
    if (state->hold_used) return;

    if (hold_shape->shape == NULL) {
        hold_shape->shape = piece->src_type->shape;
        hold_shape->width = piece->src_type->width;
        hold_shape->height = piece->src_type->height;

        spawn_piece(state);
    } else {
        swap_shape(state);
    }
    state->hold_used = 1;
}

void render_hold(GameState *state) {
    int y_offset = (height / 2) - (BOARD_HEIGHT * 0.5) + 1;
    int x_offset = (width / 2) - (BOARD_WIDTH * BLOCK_MULT_X * 0.5) - 10;

    Shape *shape = &state->hold_shape;

    if (!shape || !shape->shape) return;
    for (int i = 0; i < shape->height; i++) {
        for (int j = 0; j < shape->width; j++) {
            printf("\e[%d;%dH%s",
                   y_offset + i,
                   x_offset + (j * BLOCK_MULT_X),
                   shape->shape[i * shape->width + j] ? "[]" : "  ");
        }
    }
}

void render_score(GameState *state) {
    printf("\e[%d;%dHLEVEL UP! %d", 4, width / 2 - 7, state->level);
    printf("\e[%d;%dHSCORE: %d", 3, width/2 - 7, state->score);
}

void render_game_over(GameState *state) {
    printf("\e[%d;%dH%s",  6, width / 2 - 14/2 + 2, "==============");
    printf("\e[%d;%dH%s",  7, width / 2 - 14/2 + 2, "| GAME OVER! |");
    printf("\e[%d;%dH%s",  8, width / 2 - 14/2 + 2, "==============");
    printf("\e[%d;%dH%s", 10, width / 2 - 18/2 + 2, "Press R to Restart");
    printf("\e[%d;%dH%s", 11, width / 2 - 12/2 + 2, "or Q to Quit");
}

int calculate_score(int lvl, int lines_cleared) {
    switch (lines_cleared) {
        case 1: return 40 * (lvl + 1);
        case 2: return 100 * (lvl + 1);
        case 3: return 300 * (lvl + 1);
        case 4: return 1200 * (lvl + 1);
        default: return 0;
    }
}

void add_lines(GameState *state, int cleared) {
    state->total_lines += cleared;
    state->score += calculate_score(state->level, cleared);

    int start_level = 0; // TEMP
    int new_level = start_level + state->total_lines / 10;
    if (new_level > state->level) {
        state->level = new_level;
    }
}

int main() {
    srand(time(NULL));
    configure_terminal();
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    width = w.ws_col;
    height = w.ws_row;

    initialize_shapes(shapes);

    GameState gameState = {0};

    spawn_piece(&gameState);
    g_piece = &gameState.activePiece;

    signal(SIGINT, handle_sigint);
    double prev_time = get_time_seconds();

    gameState.running = 1;
    while (gameState.running) {
        if (kbhit()) {
            char seq[3];
            read(STDIN_FILENO, &seq[0], 1);
            if (seq[0] == '\e') { // Escape sequence
                // Check if the next two bytes exist
                if (read(STDIN_FILENO, &seq[1], 1) > 0 && seq[1] == '[') {
                    if (read(STDIN_FILENO, &seq[2], 1) > 0) {
                        switch (seq[2]) {
                            case 'A': // Up: rotate
                                rotate_shape(&gameState);
                                break;
                            case 'B': // Down
                                if (check_fall(&gameState)) {
                                    gameState.activePiece.y++;
                                } else {
                                    update_state(&gameState);
                                    spawn_piece(&gameState);
                                }
                                break;
                            case 'C': // Right
                                try_move(&gameState, 1);
                                break;
                            case 'D': // Left
                                try_move(&gameState, -1);
                                break;
                        }
                    }
                }
            } else {
                switch (seq[0]) {
                    case 'q':
                        gameState.running = 0;
                        break;
                    case 'w':
                    case 'k': // Rotate
                        rotate_shape(&gameState);
                        break;
                    case 'l': // Right
                        try_move(&gameState, 1);
                        break;
                    case 'h': // Left
                        try_move(&gameState, -1);
                        break;
                    case 'j': // Down
                        if (check_fall(&gameState)) {
                            gameState.activePiece.y++;
                        } else {
                            update_state(&gameState);
                            spawn_piece(&gameState);
                        }
                        break;
                    case ' ': // Full down
                        hard_drop(&gameState);
                        update_state(&gameState);
                        spawn_piece(&gameState);
                        break;
                    case 'c': // Hold
                        hold(&gameState);
                        break;
                }
            }
        }
        printf("\e[2J\e[H"); // clear terminal
        double now = get_time_seconds();
        if (!gameState.game_over) {
            if (now - prev_time > 0.3) {
                prev_time = now;

                if (check_fall(&gameState)) {
                    gameState.activePiece.y++;
                }
                else {
                    update_state(&gameState);
                    spawn_piece(&gameState);
                }
                add_lines(&gameState, check_clear(&gameState));
            }
        } else {
            render_game_over(&gameState);
        }
        // debug(&gameState);
        render_hold(&gameState);
        render_score(&gameState);
        render(&gameState);
        fflush(stdout);
        usleep(100000);
    }
    if (gameState.activePiece.type) {
        free(gameState.activePiece.type->shape);
        free(gameState.activePiece.type);
    }
    reset_terminal();
    free_shapes();
    return 0;
}
