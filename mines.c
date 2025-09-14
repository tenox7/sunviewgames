/* Minesweeper for SunOS 4 SunView
   Written by Claude, public domain
   Compile with: cc mines.c -o mines -lsuntool -lsunwindow -lpixrect */

#include <suntool/sunview.h>
#include <suntool/canvas.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define WINDOW_WIDTH 440
#define WINDOW_HEIGHT 460
#define BOARD_SIZE 11
#define CELL_SIZE 40
#define MINE_COUNT 12
#define NUM_COLORS 8

#define COLOR_BACKGROUND 0
#define COLOR_RED 1
#define COLOR_GREEN 2
#define COLOR_BLUE 3
#define COLOR_YELLOW 4
#define COLOR_MAGENTA 5
#define COLOR_WHITE 6
#define COLOR_BLACK 7

#define CELL_HIDDEN 0
#define CELL_REVEALED 1
#define CELL_FLAGGED 2

#define CONTENT_EMPTY 0
#define CONTENT_MINE 9

Frame frame;
Canvas canvas;
Pixwin *pw;

static int board[BOARD_SIZE][BOARD_SIZE];
static int state[BOARD_SIZE][BOARD_SIZE];
static int mines_remaining = MINE_COUNT;
static int game_over = 0;
static int game_won = 0;
static int first_click = 1;
static int cms_size;
static unsigned char red[NUM_COLORS], green[NUM_COLORS], blue[NUM_COLORS];

void init_board();
void place_mines();
void calculate_numbers();
void draw_board();
void handle_click();
void reveal_cell();
void flag_cell();
void check_win();
void setup_colors();
void handle_input();
void draw_number();

main(argc, argv)
int argc;
char **argv;
{
    srand((unsigned int)time(0));

    frame = window_create(NULL, FRAME,
        FRAME_LABEL,       "Minesweeper",
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
    red[COLOR_BACKGROUND] = 192;  green[COLOR_BACKGROUND] = 192;  blue[COLOR_BACKGROUND] = 192;
    red[COLOR_RED] = 255;         green[COLOR_RED] = 0;           blue[COLOR_RED] = 0;
    red[COLOR_GREEN] = 0;         green[COLOR_GREEN] = 255;       blue[COLOR_GREEN] = 0;
    red[COLOR_BLUE] = 0;          green[COLOR_BLUE] = 0;          blue[COLOR_BLUE] = 255;
    red[COLOR_YELLOW] = 255;      green[COLOR_YELLOW] = 255;      blue[COLOR_YELLOW] = 0;
    red[COLOR_MAGENTA] = 255;     green[COLOR_MAGENTA] = 0;       blue[COLOR_MAGENTA] = 255;
    red[COLOR_WHITE] = 255;       green[COLOR_WHITE] = 255;       blue[COLOR_WHITE] = 255;
    red[COLOR_BLACK] = 0;         green[COLOR_BLACK] = 0;         blue[COLOR_BLACK] = 0;

    pw_setcmsname(pw, "minesweeper_cms");
    pw_putcolormap(pw, 0, NUM_COLORS, red, green, blue);
}

void init_board()
{
    int i, j;

    for (i = 0; i < BOARD_SIZE; i++) {
        for (j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = CONTENT_EMPTY;
            state[i][j] = CELL_HIDDEN;
        }
    }
    mines_remaining = MINE_COUNT;
    game_over = 0;
    game_won = 0;
    first_click = 1;
}

void place_mines(avoid_x, avoid_y)
int avoid_x, avoid_y;
{
    int mines_placed = 0;
    int x, y;

    while (mines_placed < MINE_COUNT) {
        x = rand() % BOARD_SIZE;
        y = rand() % BOARD_SIZE;

        if (board[y][x] != CONTENT_MINE &&
            !(x >= avoid_x - 1 && x <= avoid_x + 1 &&
              y >= avoid_y - 1 && y <= avoid_y + 1)) {
            board[y][x] = CONTENT_MINE;
            mines_placed++;
        }
    }

    calculate_numbers();
}

void calculate_numbers()
{
    int i, j, di, dj, count;

    for (i = 0; i < BOARD_SIZE; i++) {
        for (j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] != CONTENT_MINE) {
                count = 0;
                for (di = -1; di <= 1; di++) {
                    for (dj = -1; dj <= 1; dj++) {
                        if (i + di >= 0 && i + di < BOARD_SIZE &&
                            j + dj >= 0 && j + dj < BOARD_SIZE &&
                            board[i + di][j + dj] == CONTENT_MINE) {
                            count++;
                        }
                    }
                }
                board[i][j] = count;
            }
        }
    }
}

void draw_number(x, y, num, color)
int x, y, num, color;
{
    char num_str[2];

    sprintf(num_str, "%d", num);
    pw_text(pw, x + CELL_SIZE/2 - 4, y + CELL_SIZE/2 + 4,
            PIX_SRC | PIX_COLOR(color), 0, num_str);
}

void draw_board()
{
    int i, j, x, y;
    char info_str[80];

    pw_writebackground(pw, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, PIX_SRC | PIX_COLOR(COLOR_BACKGROUND));

    for (i = 0; i < BOARD_SIZE; i++) {
        for (j = 0; j < BOARD_SIZE; j++) {
            x = j * CELL_SIZE;
            y = i * CELL_SIZE;

            /* Draw cell border */
            pw_vector(pw, x, y, x + CELL_SIZE, y, PIX_SRC | PIX_COLOR(COLOR_BLACK), 1);
            pw_vector(pw, x, y, x, y + CELL_SIZE, PIX_SRC | PIX_COLOR(COLOR_BLACK), 1);

            if (state[i][j] == CELL_REVEALED) {
                if (board[i][j] == CONTENT_MINE) {
                    /* Draw mine as red circle */
                    pw_writebackground(pw, x+1, y+1, CELL_SIZE-1, CELL_SIZE-1,
                                     PIX_SRC | PIX_COLOR(COLOR_RED));
                    pw_text(pw, x + CELL_SIZE/2 - 4, y + CELL_SIZE/2 + 4,
                            PIX_SRC | PIX_COLOR(COLOR_BLACK), 0, "*");
                } else {
                    /* Draw revealed empty cell */
                    pw_writebackground(pw, x+1, y+1, CELL_SIZE-1, CELL_SIZE-1,
                                     PIX_SRC | PIX_COLOR(COLOR_WHITE));
                    if (board[i][j] > 0) {
                        char num_str[2];
                        sprintf(num_str, "%d", board[i][j]);
                        /* Clear text area first */
                        pw_writebackground(pw, x + CELL_SIZE/2 - 8, y + CELL_SIZE/2 - 4,
                                         16, 12, PIX_SRC | PIX_COLOR(COLOR_WHITE));
                        pw_text(pw, x + CELL_SIZE/2 - 4, y + CELL_SIZE/2 + 4,
                                PIX_SRC | PIX_COLOR(COLOR_BLACK), 0, num_str);
                    }
                }
            } else if (state[i][j] == CELL_FLAGGED) {
                /* Draw flag as yellow background with F */
                pw_writebackground(pw, x+1, y+1, CELL_SIZE-1, CELL_SIZE-1,
                                 PIX_SRC | PIX_COLOR(COLOR_YELLOW));
                pw_text(pw, x + CELL_SIZE/2 - 4, y + CELL_SIZE/2 + 4,
                        PIX_SRC | PIX_COLOR(COLOR_BLACK), 0, "F");
            } else {
                /* Draw hidden cell */
                pw_writebackground(pw, x+1, y+1, CELL_SIZE-1, CELL_SIZE-1,
                                 PIX_SRC | PIX_COLOR(COLOR_BACKGROUND));
            }
        }
    }

    /* Draw bottom border */
    pw_vector(pw, 0, BOARD_SIZE * CELL_SIZE, BOARD_SIZE * CELL_SIZE,
              BOARD_SIZE * CELL_SIZE, PIX_SRC | PIX_COLOR(COLOR_BLACK), 1);
    pw_vector(pw, BOARD_SIZE * CELL_SIZE, 0, BOARD_SIZE * CELL_SIZE,
              BOARD_SIZE * CELL_SIZE, PIX_SRC | PIX_COLOR(COLOR_BLACK), 1);

    sprintf(info_str, "Mines: %d", mines_remaining);
    pw_text(pw, 10, WINDOW_HEIGHT - 30, PIX_SRC | PIX_COLOR(COLOR_BLACK), 0, info_str);

    if (game_over) {
        if (game_won) {
            pw_text(pw, 10, WINDOW_HEIGHT - 10, PIX_SRC | PIX_COLOR(COLOR_GREEN), 0,
                    "You Won! Click to restart");
        } else {
            pw_text(pw, 10, WINDOW_HEIGHT - 10, PIX_SRC | PIX_COLOR(COLOR_RED), 0,
                    "Game Over! Click to restart");
        }
    } else {
        pw_text(pw, 10, WINDOW_HEIGHT - 10, PIX_SRC | PIX_COLOR(COLOR_BLACK), 0,
                "Left click: reveal, Right click: flag");
    }
}

void reveal_cell(x, y)
int x, y;
{
    int i, j;

    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE ||
        state[y][x] == CELL_REVEALED || state[y][x] == CELL_FLAGGED) {
        return;
    }

    state[y][x] = CELL_REVEALED;

    if (board[y][x] == CONTENT_MINE) {
        game_over = 1;
        /* Reveal all mines */
        for (i = 0; i < BOARD_SIZE; i++) {
            for (j = 0; j < BOARD_SIZE; j++) {
                if (board[i][j] == CONTENT_MINE) {
                    state[i][j] = CELL_REVEALED;
                }
            }
        }
        return;
    }

    /* If empty cell, reveal adjacent cells */
    if (board[y][x] == 0) {
        for (i = -1; i <= 1; i++) {
            for (j = -1; j <= 1; j++) {
                reveal_cell(x + j, y + i);
            }
        }
    }
}

void flag_cell(x, y)
int x, y;
{
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE ||
        state[y][x] == CELL_REVEALED) {
        return;
    }

    if (state[y][x] == CELL_FLAGGED) {
        state[y][x] = CELL_HIDDEN;
        mines_remaining++;
    } else {
        state[y][x] = CELL_FLAGGED;
        mines_remaining--;
    }
}

void check_win()
{
    int i, j, hidden_count = 0;

    for (i = 0; i < BOARD_SIZE; i++) {
        for (j = 0; j < BOARD_SIZE; j++) {
            if (state[i][j] == CELL_HIDDEN) {
                hidden_count++;
            }
        }
    }

    if (hidden_count == MINE_COUNT) {
        game_won = 1;
        game_over = 1;
    }
}

void handle_click(x, y, right_click)
int x, y, right_click;
{
    if (game_over) {
        init_board();
        draw_board();
        return;
    }

    if (first_click && !right_click) {
        place_mines(x, y);
        first_click = 0;
    }

    if (right_click) {
        flag_cell(x, y);
    } else {
        reveal_cell(x, y);
    }

    if (!game_over) {
        check_win();
    }

    draw_board();
}

void handle_input(window, event, arg)
Window window;
Event *event;
caddr_t arg;
{
    int x, y;

    if ((event_action(event) == MS_LEFT || event_action(event) == MS_RIGHT) &&
        event_is_down(event)) {
        x = event_x(event) / CELL_SIZE;
        y = event_y(event) / CELL_SIZE;
        if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE) {
            handle_click(x, y, event_action(event) == MS_RIGHT);
        }
    } else if (event_is_ascii(event) && event_id(event) == 'q') {
        exit(0);
    }
}