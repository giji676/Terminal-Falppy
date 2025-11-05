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

void handle_sigint(int sig) {
    reset_terminal();
    _exit(0);
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

int shape_o[2*2] = {
    1,1,
    1,1
};

int shape_l[4*1] = {
    1,
    1,
    1,
    1
};

#define NUM_SHAPES 5

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
    shapes[4]->shape = shape_l;
    shapes[4]->width = 1;
    shapes[4]->height = 4;
}

typedef struct {
    Shape *type;
    int x, y;
    int rotation;
} ActivePiece;

ActivePiece spawn_piece(Shape *shapes[]) {
    ActivePiece p;
    p.type = shapes[rand() % NUM_SHAPES];
    p.x = BOARD_WIDTH / 2 - p.type->width / 2;
    p.y = 0;
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
}

void update_state(int board[BOARD_HEIGHT][BOARD_WIDTH], ActivePiece *piece) {
    Shape *shape = piece->type;
    if (piece->y + shape->height < BOARD_HEIGHT) {
        // Clear old shape pposition on board
        for (int y = 0; y < shape->height; y++) {
            for (int x = 0; x < shape->width; x++) {
                if (shape->shape[y * shape->width + x]) {
                    board[piece->y + y][piece->x + x] = 0;
                }
            }
        }
        piece->y++;
        // Draw new shape pposition on board
        for (int y = 0; y < shape->height; y++) {
            for (int x = 0; x < shape->width; x++) {
                board[piece->y + y][piece->x + x] = shape->shape[y * shape->width + x];
            }
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

    Shape *shapes[NUM_SHAPES];
    initialize_shapes(shapes);

    int board[BOARD_HEIGHT][BOARD_WIDTH] = {0};
    ActivePiece piece = spawn_piece(shapes);

    signal(SIGINT, handle_sigint);
    double prev_time = get_time_seconds();

    char c;

    int running = 1;
    while (running) {
        double now = get_time_seconds();
        if (now - prev_time > 0.5) {
            prev_time = now;
            update_state(board, &piece);
        }
        render(board, &piece);
        usleep(100000);
    }
}
