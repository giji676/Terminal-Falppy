#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define BLOCK_MULT_X 2 
#define BLOCK_MULT_Y 1

struct termios oldt, newt;
int width, height;

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

int shape_s[2*3] = {
    0,1,1,
    1,1,0
};

int shape_z[2*3] = {
    1,1,0,
    0,1,1
};

int shape_t[2*3] = {
    1,1,1,
    0,1,0
};

int shape_l[3*2] = {
    1,0,
    1,0,
    1,1,
};

int shape_j[3*2] = {
    0,1,
    0,1,
    1,1,
};

int shape_o[2*2] = {
    1,1,
    1,1
};

int shape_i[4*1] = {
    1,
    1,
    1,
    1
};

#define NUM_SHAPES 7

typedef struct {
    int width, height;
    int *shape;
} Shape;

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

typedef struct {
    Shape *type;
    int x, y;
} ActivePiece;

ActivePiece spawn_piece(Shape *shapes[], int *hold_used) {
    ActivePiece p;
    p.type = shapes[rand() % NUM_SHAPES];
    p.x = BOARD_WIDTH / 2 - p.type->width / 2;
    p.y = 0;
    *hold_used = 0;
    return p;
}

void render(int board[BOARD_HEIGHT][BOARD_WIDTH], ActivePiece *piece) {
    int y_offset = (height / 2) - (BOARD_HEIGHT * BLOCK_MULT_Y * 0.5);
    int x_offset = (width / 2) - (BOARD_WIDTH * BLOCK_MULT_X * 0.5);

    for (int i = 0; i < BOARD_HEIGHT + 2; i++) {
        if (i <= BOARD_HEIGHT) {
            printf("\e[%d;%dH\e[32m%s",
                   (i + 1) * BLOCK_MULT_Y + y_offset, 
                   x_offset,
                   "<!");
            printf("\e[%d;%dH\e[32m%s",
                   (i + 1) * BLOCK_MULT_Y + y_offset, 
                   x_offset + BOARD_WIDTH * BLOCK_MULT_X + 2,
                   "!>");
        }
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (i == BOARD_HEIGHT) {
                printf("\e[%d;%dH\e[32m%s",
                       (i + 1) * BLOCK_MULT_Y + y_offset, 
                       (j + 1) * BLOCK_MULT_X + x_offset, 
                       "==");
            } else if (i == BOARD_HEIGHT + 1) {
                printf("\e[%d;%dH\e[32m%s",
                       (i + 1) * BLOCK_MULT_Y + y_offset, 
                       (j + 1) * BLOCK_MULT_X + x_offset, 
                       "\\/");
            } else {
                if (board[i][j]) {
                    printf("\e[%d;%dH\e[32m%s",
                           (i + 1) * BLOCK_MULT_Y + y_offset, 
                           (j + 1) * BLOCK_MULT_X + x_offset, 
                           "[]");
                } else {
                    printf("\e[%d;%dH\e[32m%s",
                           (i + 1) * BLOCK_MULT_Y + y_offset, 
                           (j + 1) * BLOCK_MULT_X + x_offset, 
                           " .");
                }
            }
        }
    }

    Shape *shape = piece->type;

    for (int y = 0; y < shape->height; y++) {
        for (int x = 0; x < shape->width; x++) {
            if (shape->shape[y * shape->width + x]) {
                printf("\e[%d;%dH\e[32m%s",
                       (piece->y + 1 + y) * BLOCK_MULT_Y + y_offset,
                       (piece->x + 1 + x) * BLOCK_MULT_X + x_offset,
                       "[]");
            }
        }
    }
}

Shape *shapes[NUM_SHAPES];

void free_shapes() {
    for (int i = 0; i < NUM_SHAPES; i++) {
        free(shapes[i]);
    }
}

void handle_sigint(int sig) {
    free_shapes();
    reset_terminal();
    _exit(0);
}

int check_fall(int board[BOARD_HEIGHT][BOARD_WIDTH], ActivePiece *piece) {
    Shape *shape = piece->type;

    for (int y = 0; y < shape->height; y++) {
        for (int x = 0; x < shape->width; x++) {
            if (piece->y + y == BOARD_HEIGHT - 1) {
                return 0;
            }
            if (!shape->shape[y * shape->width + x]) {
                continue;
            }
            if (board[piece->y + y + 1][piece->x + x]) {
                return 0;
            }
        }
    }
    return 1;
}

void hard_drop(int board[BOARD_HEIGHT][BOARD_WIDTH], ActivePiece *piece) {
    while (check_fall(board, piece)) {
        piece->y++;
    }
}

void try_move(int dir, int board[BOARD_HEIGHT][BOARD_WIDTH], ActivePiece *piece) {
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
            if (board[piece->y + y][piece->x + x + dir]) {
                return;
            }
        }
    }
    piece->x += dir;
}


void update_state(int board[BOARD_HEIGHT][BOARD_WIDTH], ActivePiece *piece) {
    Shape *shape = piece->type;
    for (int y = 0; y < shape->height; y++) {
        for (int x = 0; x < shape->width; x++) {
            if (shape->shape[y * shape->width + x]) {
                board[piece->y+y][piece->x+x] = 1;
            }
        }
    }
}

void debug(int board[BOARD_HEIGHT][BOARD_WIDTH], ActivePiece *piece) {
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            printf("\e[%d;%dH%i", i, j, board[i][j]);
        }
    }
    Shape *shape = piece->type;
    for (int y = 0; y < shape->height; y++) {
        for (int x = 0; x < shape->width; x++) {
            int val = shape->shape[y * shape->width + x] * 2;
            if (val != 0) {
                printf("\e[%d;%dH%i",
                       piece->y + y,
                       piece->x + x,
                       val);
            }
        }
    }
    printf("\e[%d;%dH(%i,%i)", BOARD_HEIGHT+1, 1, piece->y, piece->x);
}

void check_clear(int board[BOARD_HEIGHT][BOARD_WIDTH]) {
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        int full = 1;
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (!board[i][j]) {
                full = 0;
                break;
            }
        }
        if (full) {
            for (int k = i; k > 0; k--) {
                for (int j = 0; j < BOARD_WIDTH; j++) {
                    board[k][j] = board[k - 1][j];
                }
            }
            for (int j = 0; j < BOARD_WIDTH; j++) {
                board[0][j] = 0;
            }
            i--;
        }
    }
}

void rotate_shape(int board[BOARD_HEIGHT][BOARD_WIDTH], ActivePiece *piece) {
    Shape *shape = piece->type;
    int new_w = shape->height;
    int new_h = shape->width;

    int rotated[new_h * new_w];

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

            if (board[new_y][new_x])
                return;
        }
    }

    for (int i = 0; i < shape->width * shape->height; i++)
        shape->shape[i] = rotated[i];

    shape->width = new_w;
    shape->height = new_h;
}

void hold(ActivePiece *piece, Shape *shape, int *hold_used) {
    if (*hold_used) return;
    Shape temp;

    if (shape->shape == NULL) {
        shape->shape = piece->type->shape;
        shape->width = piece->type->width;
        shape->height = piece->type->height;

        *piece = spawn_piece(shapes, hold_used);
        *hold_used = 1;
    } else {
        temp.shape = piece->type->shape;
        temp.width = piece->type->width;
        temp.height = piece->type->height;

        piece->type->shape = shape->shape;
        piece->type->width = shape->width;
        piece->type->height = shape->height;
        piece->x = BOARD_WIDTH / 2 - piece->type->width / 2;
        piece->y = 0;

        shape->shape = temp.shape;
        shape->width = temp.width;
        shape->height = temp.height;
        *hold_used = 1;
    }
}

void render_hold(Shape *shape) {
    if (!shape || !shape->shape) return;
    for (int i = 0; i < shape->height; i++) {
        for (int j = 0; j < shape->width; j++) {
            printf("\e[%d;%dH%i", 1+i, BOARD_WIDTH+2+j, shape->shape[i * shape->width + j]);
        }
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
    int hold_used = 0;

    int board[BOARD_HEIGHT][BOARD_WIDTH] = {0};
    ActivePiece piece = spawn_piece(shapes, &hold_used);
    Shape hold_shape;
    hold_shape.shape = NULL;

    signal(SIGINT, handle_sigint);
    double prev_time = get_time_seconds();

    int running = 1;
    while (running) {
        printf("\e[2J\e[H"); // clear terminal
        if (kbhit()) {
            char seq[3];
            read(STDIN_FILENO, &seq[0], 1);
            if (seq[0] == '\e') { // Escape sequence
                // Check if the next two bytes exist
                if (read(STDIN_FILENO, &seq[1], 1) > 0 && seq[1] == '[') {
                    if (read(STDIN_FILENO, &seq[2], 1) > 0) {
                        switch (seq[2]) {
                            case 'A': // Up: rotate
                                rotate_shape(board, &piece);
                                break;
                            case 'B': // Down
                                if (check_fall(board, &piece)) {
                                    piece.y++;
                                } else {
                                    update_state(board, &piece);
                                    piece = spawn_piece(shapes, &hold_used);
                                }
                                break;
                            case 'C': // Right
                                try_move(1, board, &piece);
                                break;
                            case 'D': // Left
                                try_move(-1, board, &piece);
                                break;
                        }
                    }
                }
            } else {
                switch (seq[0]) {
                    case 'q':
                        running = 0;
                        break;
                    case 'w':
                    case 'k': // Rotate
                        rotate_shape(board, &piece);
                        break;
                    case 'l': // Right
                        try_move(1, board, &piece);
                        break;
                    case 'h': // Left
                        try_move(-1, board, &piece);
                        break;
                    case 'j': // Down
                        if (check_fall(board, &piece)) {
                            piece.y++;
                        } else {
                            update_state(board, &piece);
                            piece = spawn_piece(shapes, &hold_used);
                        }
                        break;
                    case ' ': // Full down
                        hard_drop(board, &piece);
                        update_state(board, &piece);
                        piece = spawn_piece(shapes, &hold_used);
                        break;
                    case 'c': // Hold
                        hold(&piece, &hold_shape, &hold_used);
                        break;
                }
            }
        }
        double now = get_time_seconds();
        if (now - prev_time > 0.3) {
            prev_time = now;

            if (check_fall(board, &piece)) {
                piece.y++;
            }
            else {
                update_state(board, &piece);
                piece = spawn_piece(shapes, &hold_used);
            }
            check_clear(board);
        }
        debug(board, &piece);
        render_hold(&hold_shape);
        render(board, &piece);
        fflush(stdout);
        usleep(100000);
    }
}
