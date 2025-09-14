/* Bubble Breaker for SunOS 4 SunView
   Written by Claude, public domain
   Compile with: cc bubble_breaker.c -o bubble_breaker -lsuntool -lsunwindow -lpixrect */

#include <suntool/sunview.h>
#include <suntool/canvas.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define WINDOW_WIDTH 440
#define WINDOW_HEIGHT 440
#define BOARD_SIZE 11
#define CELL_SIZE 40
#define COLOR_COUNT 5
#define NUM_COLORS 8

#define COLOR_BACKGROUND 0
#define COLOR_RED 1
#define COLOR_GREEN 2
#define COLOR_BLUE 3
#define COLOR_YELLOW 4
#define COLOR_MAGENTA 5
#define COLOR_WHITE 6
#define COLOR_BLACK 7

Frame frame;
Canvas canvas;
Pixwin *pw;

static int board[BOARD_SIZE][BOARD_SIZE];
static int score = 0;
static int game_over = 0;
static int cms_size;
static unsigned char red[NUM_COLORS], green[NUM_COLORS], blue[NUM_COLORS];

static int colors[] = {
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_YELLOW,
    COLOR_MAGENTA
};

void init_board();
void draw_board();
void handle_click();
int *find_cluster();
void apply_gravity();
void shift_columns();
int is_game_over();
void setup_colors();
void handle_input();

main(argc, argv)
int argc;
char **argv;
{
    srand((unsigned int)time(0));

    frame = window_create(NULL, FRAME,
        FRAME_LABEL,       "Bubble Breaker",
        WIN_WIDTH,         WINDOW_WIDTH,
        WIN_HEIGHT,        WINDOW_HEIGHT,
        0);

    canvas = window_create(frame, CANVAS,
        WIN_WIDTH,         WINDOW_WIDTH,
        WIN_HEIGHT,        WINDOW_HEIGHT,
        WIN_EVENT_PROC,    handle_input,
        WIN_CONSUME_KBD_EVENTS, WIN_UP_EVENTS | WIN_ASCII_EVENTS,
        0);

    pw = canvas_pixwin(canvas);
    if (pw == NULL) {
        fprintf(stderr, "Failed to get pixwin\n");
        exit(1);
    }

    setup_colors();
    init_board();
    draw_board();

    window_fit(frame);
    window_main_loop(frame);
    exit(0);
}

void setup_colors()
{
    cms_size = NUM_COLORS;
    red[COLOR_BACKGROUND] = 0;   green[COLOR_BACKGROUND] = 0;   blue[COLOR_BACKGROUND] = 0;
    red[COLOR_RED] = 255;        green[COLOR_RED] = 0;          blue[COLOR_RED] = 0;
    red[COLOR_GREEN] = 0;        green[COLOR_GREEN] = 255;      blue[COLOR_GREEN] = 0;
    red[COLOR_BLUE] = 0;         green[COLOR_BLUE] = 0;         blue[COLOR_BLUE] = 255;
    red[COLOR_YELLOW] = 255;     green[COLOR_YELLOW] = 255;     blue[COLOR_YELLOW] = 0;
    red[COLOR_MAGENTA] = 255;    green[COLOR_MAGENTA] = 0;      blue[COLOR_MAGENTA] = 255;
    red[COLOR_WHITE] = 255;      green[COLOR_WHITE] = 255;      blue[COLOR_WHITE] = 255;
    red[COLOR_BLACK] = 0;        green[COLOR_BLACK] = 0;        blue[COLOR_BLACK] = 0;

    pw_setcmsname(pw, "bubble_breaker_cms");
    pw_putcolormap(pw, 0, NUM_COLORS, red, green, blue);
}

void init_board()
{
    int i, j;
    
    for (i = 0; i < BOARD_SIZE; i++) {
        for (j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = rand() % COLOR_COUNT;
        }
    }
    score = 0;
    game_over = 0;
}

void draw_circle(x0, y0, radius, color)
int x0, y0, radius, color;
{
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y)
    {
        pw_vector(pw, x0 + x, y0 + y, x0 + x, y0 + y, PIX_SRC | PIX_COLOR(color), 1);
        pw_vector(pw, x0 + y, y0 + x, x0 + y, y0 + x, PIX_SRC | PIX_COLOR(color), 1);
        pw_vector(pw, x0 - y, y0 + x, x0 - y, y0 + x, PIX_SRC | PIX_COLOR(color), 1);
        pw_vector(pw, x0 - x, y0 + y, x0 - x, y0 + y, PIX_SRC | PIX_COLOR(color), 1);
        pw_vector(pw, x0 - x, y0 - y, x0 - x, y0 - y, PIX_SRC | PIX_COLOR(color), 1);
        pw_vector(pw, x0 - y, y0 - x, x0 - y, y0 - x, PIX_SRC | PIX_COLOR(color), 1);
        pw_vector(pw, x0 + y, y0 - x, x0 + y, y0 - x, PIX_SRC | PIX_COLOR(color), 1);
        pw_vector(pw, x0 + x, y0 - y, x0 + x, y0 - y, PIX_SRC | PIX_COLOR(color), 1);

        if (err <= 0)
        {
            y += 1;
            err += 2*y + 1;
        }
        if (err > 0)
        {
            x -= 1;
            err -= 2*x + 1;
        }
    }
}

void fill_circle(x0, y0, radius, color)
int x0, y0, radius, color;
{
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y)
    {
        pw_vector(pw, x0 - x, y0 + y, x0 + x, y0 + y, PIX_SRC | PIX_COLOR(color), 1);
        pw_vector(pw, x0 - y, y0 + x, x0 + y, y0 + x, PIX_SRC | PIX_COLOR(color), 1);
        pw_vector(pw, x0 - x, y0 - y, x0 + x, y0 - y, PIX_SRC | PIX_COLOR(color), 1);
        pw_vector(pw, x0 - y, y0 - x, x0 + y, y0 - x, PIX_SRC | PIX_COLOR(color), 1);

        if (err <= 0)
        {
            y += 1;
            err += 2*y + 1;
        }
        if (err > 0)
        {
            x -= 1;
            err -= 2*x + 1;
        }
    }
}

void draw_board()
{
    int i, j;
    char score_str[50];

    pw_writebackground(pw, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, PIX_SRC | PIX_COLOR(COLOR_BACKGROUND));

    for (i = 0; i < BOARD_SIZE; i++) {
        for (j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] != -1) {
                fill_circle(j*CELL_SIZE + CELL_SIZE/2, 
                            i*CELL_SIZE + CELL_SIZE/2, 
                            CELL_SIZE/2 - 1, 
                            colors[board[i][j]]);
            }
        }
    }

    sprintf(score_str, "Score: %d", score);
    pw_text(pw, 10, WINDOW_HEIGHT - 10, PIX_SRC | PIX_COLOR(COLOR_WHITE), 0, score_str);
    
    if (game_over) {
        pw_text(pw, WINDOW_WIDTH/2 - 80, WINDOW_HEIGHT/2, PIX_SRC | PIX_COLOR(COLOR_WHITE), 0, "Game Over! Click to restart");
    }
}

void handle_click(x, y)
int x, y;
{
    int color, cluster_size, i;
    int *cluster;

    if (game_over) {
        init_board();
        draw_board();
        return;
    }

    color = board[y][x];
    if (color == -1) return;

    cluster = find_cluster(x, y, color, &cluster_size);
    if (cluster_size < 2) {
        free(cluster);
        return;
    }

    score += cluster_size * cluster_size;

    for (i = 0; i < cluster_size; i++) {
        board[cluster[i] / BOARD_SIZE][cluster[i] % BOARD_SIZE] = -1;
    }
    free((char *)cluster);

    apply_gravity();
    shift_columns();

    if (is_game_over()) {
        game_over = 1;
    }

    draw_board();
}

int *find_cluster(x, y, color, size)
int x, y, color, *size;
{
    int *cluster, *stack;
    int stack_size, cluster_size, dx, dy;
    int visited[BOARD_SIZE][BOARD_SIZE];
    int i, j, new_x, new_y;

    for (i = 0; i < BOARD_SIZE; i++)
        for (j = 0; j < BOARD_SIZE; j++)
            visited[i][j] = 0;

    cluster = (int *)malloc(BOARD_SIZE * BOARD_SIZE * sizeof(int));
    stack = (int *)malloc(BOARD_SIZE * BOARD_SIZE * sizeof(int));
    stack_size = 0;
    cluster_size = 0;

    stack[stack_size++] = y * BOARD_SIZE + x;

    while (stack_size > 0) {
        stack_size--;
        i = stack[stack_size] / BOARD_SIZE;
        j = stack[stack_size] % BOARD_SIZE;

        if (i < 0 || i >= BOARD_SIZE || j < 0 || j >= BOARD_SIZE || 
            board[i][j] != color || visited[i][j]) {
            continue;
        }

        visited[i][j] = 1;
        cluster[cluster_size++] = i * BOARD_SIZE + j;

        for (dx = -1; dx <= 1; dx++) {
            for (dy = -1; dy <= 1; dy++) {
                if (dx == 0 || dy == 0) {
                    new_x = j + dx;
                    new_y = i + dy;
                    stack[stack_size++] = new_y * BOARD_SIZE + new_x;
                }
            }
        }
    }

    free((char *)stack);
    *size = cluster_size;
    return cluster;
}

void apply_gravity()
{
    int i, j, k;

    for (j = 0; j < BOARD_SIZE; j++) {
        k = BOARD_SIZE - 1;
        for (i = BOARD_SIZE - 1; i >= 0; i--) {
            if (board[i][j] != -1) {
                board[k][j] = board[i][j];
                k--;
            }
        }
        for (; k >= 0; k--) {
            board[k][j] = -1;
        }
    }
}

void shift_columns()
{
    int i, j, k, empty;

    for (j = 0; j < BOARD_SIZE - 1; j++) {
        if (board[BOARD_SIZE-1][j] == -1) {
            empty = 1;
            for (k = j + 1; k < BOARD_SIZE; k++) {
                if (board[BOARD_SIZE-1][k] != -1) {
                    empty = 0;
                    for (i = 0; i < BOARD_SIZE; i++) {
                        board[i][j] = board[i][k];
                        board[i][k] = -1;
                    }
                    break;
                }
            }
            if (empty) break;
        }
    }
}

int is_game_over()
{
    int i, j, cluster_size;
    int *cluster;

    for (i = 0; i < BOARD_SIZE; i++) {
        for (j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] != -1) {
                cluster = find_cluster(j, i, board[i][j], &cluster_size);
                free((char *)cluster);
                if (cluster_size >= 2) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

void handle_input(window, event, arg)
Window window;
Event *event;
caddr_t arg;
{
    int x, y;

    if (event_action(event) == MS_LEFT && event_is_down(event)) {
        x = event_x(event) / CELL_SIZE;
        y = event_y(event) / CELL_SIZE;
        if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE) {
            handle_click(x, y);
        }
    } else if (event_is_ascii(event) && event_id(event) == 'q') {
        exit(0);
    }
}