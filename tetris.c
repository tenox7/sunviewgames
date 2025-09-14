/* sunview tetris, written by claude, public domain
   cc tetris.c -o tetris -lsuntool -lsunwindow -lpixrect */
#include <suntool/sunview.h>
#include <suntool/canvas.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>

#define WINDOW_WIDTH 300
#define WINDOW_HEIGHT 600
#define GRID_WIDTH 10
#define GRID_HEIGHT 20
#define BLOCK_SIZE 30
#define FALL_INTERVAL 1  /* seconds between each fall */
#define NUM_COLORS 8

Frame frame;
Canvas canvas;
Pixwin *pw;

int grid[GRID_HEIGHT][GRID_WIDTH];
int current_piece[4][4];
int current_piece_x, current_piece_y;
int current_piece_color;
int score;
int cms_size;
unsigned char red[NUM_COLORS], green[NUM_COLORS], blue[NUM_COLORS];

void init_game();
void draw_grid();
void create_new_piece();
void draw_piece();
int check_collision();
void merge_piece();
void clear_lines();
void rotate_piece();
void handle_input();
void draw_filled_block();
Notify_value game_tick();

main(argc, argv)
int argc;
char **argv;
{
    struct itimerval timer;

    srand(time(0));
    frame = window_create(NULL, FRAME,
        FRAME_LABEL,       "Tetris SunView",
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

    /* Set up colormap */
    cms_size = NUM_COLORS;
    red[0] = 255; green[0] = 255; blue[0] = 255; /* White */
    red[1] = 0;   green[1] = 0;   blue[1] = 0;   /* Black */
    red[2] = 255; green[2] = 0;   blue[2] = 0;   /* Red */
    red[3] = 0;   green[3] = 255; blue[3] = 0;   /* Green */
    red[4] = 0;   green[4] = 0;   blue[4] = 255; /* Blue */
    red[5] = 255; green[5] = 255; blue[5] = 0;   /* Yellow */
    red[6] = 255; green[6] = 0;   blue[6] = 255; /* Magenta */
    red[7] = 0;   green[7] = 255; blue[7] = 255; /* Cyan */

    pw_setcmsname(pw, "tetris_cms");
    pw_putcolormap(pw, 0, NUM_COLORS, red, green, blue);

    window_fit(frame);

    init_game();
    create_new_piece();
    draw_grid();

    /* Set up the timer */
    timer.it_value.tv_sec = FALL_INTERVAL;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = FALL_INTERVAL;
    timer.it_interval.tv_usec = 0;
    notify_set_itimer_func(frame, game_tick, ITIMER_REAL, &timer, NULL);

    window_main_loop(frame);
    exit(0);
}

void init_game()
{
    int i, j;
    
    for (i = 0; i < GRID_HEIGHT; i++) {
        for (j = 0; j < GRID_WIDTH; j++) {
            grid[i][j] = 0;
        }
    }
    score = 0;
}

void draw_filled_block(x, y, color)
int x, y, color;
{
    pw_rop(pw, x * BLOCK_SIZE, y * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE, 
           PIX_SRC | PIX_COLOR(color), NULL, 0, 0);
}

void draw_grid()
{
    int i, j;
    char str[20];

    pw_writebackground(pw, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, PIX_SRC);

    for (i = 0; i < GRID_HEIGHT; i++) {
        for (j = 0; j < GRID_WIDTH; j++) {
            if (grid[i][j]) {
                draw_filled_block(j, i, grid[i][j]);
            }
            pw_vector(pw, j*BLOCK_SIZE, i*BLOCK_SIZE, j*BLOCK_SIZE+BLOCK_SIZE, i*BLOCK_SIZE, PIX_SRC, 1);
            pw_vector(pw, j*BLOCK_SIZE, i*BLOCK_SIZE, j*BLOCK_SIZE, i*BLOCK_SIZE+BLOCK_SIZE, PIX_SRC, 1);
        }
    }

    draw_piece();

    sprintf(str, "Score: %d", score);
    pw_text(pw, 10, WINDOW_HEIGHT - 10, PIX_SRC, 0, str);
}

void create_new_piece()
{
    static int pieces[7][4][4] = {
        1,1,1,1, 0,0,0,0, 0,0,0,0, 0,0,0,0, /* I */
        1,1,1,0, 0,1,0,0, 0,0,0,0, 0,0,0,0, /* T */
        1,1,1,0, 1,0,0,0, 0,0,0,0, 0,0,0,0, /* L */
        1,1,1,0, 0,0,1,0, 0,0,0,0, 0,0,0,0, /* J */
        1,1,0,0, 1,1,0,0, 0,0,0,0, 0,0,0,0, /* O */
        0,1,1,0, 1,1,0,0, 0,0,0,0, 0,0,0,0, /* S */
        1,1,0,0, 0,1,1,0, 0,0,0,0, 0,0,0,0  /* Z */
    };
    static int piece_colors[7] = {2, 3, 4, 5, 6, 7, 2}; /* Colors for each piece type */
    int piece;

    piece = rand() % 7;
    memcpy(current_piece, pieces[piece], sizeof(current_piece));
    current_piece_color = piece_colors[piece];
    current_piece_x = GRID_WIDTH / 2 - 2;
    current_piece_y = 0;
}

void draw_piece()
{
    int i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (current_piece[i][j]) {
                draw_filled_block(current_piece_x + j, current_piece_y + i, current_piece_color);
            }
        }
    }
}

int check_collision()
{
    int i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (current_piece[i][j]) {
                if (current_piece_y + i >= GRID_HEIGHT || 
                    current_piece_x + j < 0 || 
                    current_piece_x + j >= GRID_WIDTH ||
                    grid[current_piece_y + i][current_piece_x + j]) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

void merge_piece()
{
    int i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (current_piece[i][j]) {
                grid[current_piece_y + i][current_piece_x + j] = current_piece_color;
            }
        }
    }
}

void clear_lines()
{
    int i, j, k, full_line;

    for (i = GRID_HEIGHT - 1; i >= 0; i--) {
        full_line = 1;
        for (j = 0; j < GRID_WIDTH; j++) {
            if (!grid[i][j]) {
                full_line = 0;
                break;
            }
        }
        if (full_line) {
            for (k = i; k > 0; k--) {
                memcpy(grid[k], grid[k-1], sizeof(grid[k]));
            }
            memset(grid[0], 0, sizeof(grid[0]));
            score += 100;
            i++;  /* Check the same row again */
        }
    }
}

void rotate_piece()
{
    int temp[4][4];
    int i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            temp[i][j] = current_piece[3-j][i];
        }
    }

    memcpy(current_piece, temp, sizeof(current_piece));
}

void handle_input(window, event, arg)
Window window;
Event *event;
caddr_t arg;
{
    unsigned short key_id;

    key_id = event_id(event);

    if (event_is_ascii(event)) {
        switch (key_id) {
            case 'q':
            case 'Q':
                exit(0);
                break;
            case 'a':
            case 'A':
                current_piece_x--;
                if (check_collision()) current_piece_x++;
                break;
            case 'd':
            case 'D':
                current_piece_x++;
                if (check_collision()) current_piece_x--;
                break;
            case 's':
            case 'S':
                current_piece_y++;
                if (check_collision()) {
                    current_piece_y--;
                    merge_piece();
                    clear_lines();
                    create_new_piece();
                }
                break;
            case 'w':
            case 'W':
                rotate_piece();
                if (check_collision()) rotate_piece();  /* Rotate back if collision */
                break;
        }
        draw_grid();
    }
}

Notify_value game_tick(client, which)
    Notify_client client;
    int which;
{
    current_piece_y++;
    if (check_collision()) {
        current_piece_y--;
        merge_piece();
        clear_lines();
        create_new_piece();
        if (check_collision()) {
            pw_text(pw, 100, 300, PIX_SRC, 0, "Game Over!");
            return NOTIFY_DONE;
        }
    }
    draw_grid();
    return NOTIFY_DONE;
}
