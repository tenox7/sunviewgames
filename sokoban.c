/* Sokoban for SunOS 4 SunView
   Written by Claude, public domain
   cc sokoban.c -o sokoban -lsuntool -lsunwindow -lpixrect */

#include <suntool/sunview.h>
#include <suntool/canvas.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "levels.h"

#define WINDOW_WIDTH 500
#define WINDOW_HEIGHT 400
#define CELL_SIZE 25
#define MAX_LEVEL_WIDTH 20
#define MAX_LEVEL_HEIGHT 20

/* Colors */
#define COLOR_BACKGROUND 0
#define COLOR_WALL 1
#define COLOR_BOX 2
#define COLOR_TARGET 3
#define COLOR_PLAYER 4
#define COLOR_BOX_ON_TARGET 5
#define COLOR_TEXT 6
#define COLOR_FLOOR 0

Frame frame;
Canvas canvas;
Pixwin *pw;

/* Game state */
int game_grid[MAX_LEVEL_HEIGHT][MAX_LEVEL_WIDTH];
int player_x, player_y;
int current_level = 0;
int level_width, level_height;
int moves = 0;
int pushes = 0;

/* Function prototypes */
void init_level();
void draw_game();
void draw_cell();
void move_player();
void check_win();
void handle_input();
void parse_level();
int can_move();
void make_move();

main(argc, argv)
int argc;
char **argv;
{
    unsigned char red[8], green[8], blue[8];

    frame = window_create(NULL, FRAME,
        FRAME_LABEL,       "Sokoban SunView",
        WIN_WIDTH,         WINDOW_WIDTH,
        WIN_HEIGHT,        WINDOW_HEIGHT,
        WIN_EVENT_PROC,    handle_input,
        0);

    canvas = window_create(frame, CANVAS,
        WIN_WIDTH,           WINDOW_WIDTH,
        WIN_HEIGHT,          WINDOW_HEIGHT,
        CANVAS_REPAINT_PROC, draw_game,
        0);

    pw = canvas_pixwin(canvas);
    if (pw == NULL) {
        fprintf(stderr, "Failed to get pixwin\n");
        exit(1);
    }

    /* Set up colors */
    red[COLOR_BACKGROUND] = 255;  green[COLOR_BACKGROUND] = 255;  blue[COLOR_BACKGROUND] = 255;  /* White */
    red[COLOR_WALL] = 80;         green[COLOR_WALL] = 60;         blue[COLOR_WALL] = 40;         /* Brown */
    red[COLOR_BOX] = 200;         green[COLOR_BOX] = 150;         blue[COLOR_BOX] = 100;         /* Light brown */
    red[COLOR_TARGET] = 200;      green[COLOR_TARGET] = 0;        blue[COLOR_TARGET] = 0;        /* Red */
    red[COLOR_PLAYER] = 0;        green[COLOR_PLAYER] = 150;      blue[COLOR_PLAYER] = 255;      /* Blue */
    red[COLOR_BOX_ON_TARGET] = 0; green[COLOR_BOX_ON_TARGET] = 200; blue[COLOR_BOX_ON_TARGET] = 0; /* Green */
    red[COLOR_TEXT] = 0;          green[COLOR_TEXT] = 0;          blue[COLOR_TEXT] = 0;          /* Black */
    red[COLOR_FLOOR] = 255;       green[COLOR_FLOOR] = 255;       blue[COLOR_FLOOR] = 255;       /* White */

    pw_setcmsname(pw, "sokoban_colormap");
    pw_putcolormap(pw, 0, 8, red, green, blue);

    window_fit(frame);
    init_level();
    draw_game();

    window_main_loop(frame);
    return 0;
}

void parse_level()
{
    char *level_data;
    int x, y, i;
    char c;

    level_data = levels[current_level].data;
    level_width = 0;
    level_height = 0;
    x = 0;
    y = 0;

    /* Clear grid */
    for (i = 0; i < MAX_LEVEL_HEIGHT; i++) {
        memset(game_grid[i], EMPTY, MAX_LEVEL_WIDTH * sizeof(int));
    }

    /* Parse level string */
    i = 0;
    while ((c = level_data[i++]) != '\0') {
        if (c == '\n') {
            if (x > level_width) level_width = x;
            y++;
            x = 0;
            continue;
        }

        switch (c) {
            case WALL:
                game_grid[y][x] = WALL;
                break;
            case BOX:
                game_grid[y][x] = BOX;
                break;
            case GOAL:
                game_grid[y][x] = GOAL;
                break;
            case PLAYER:
                game_grid[y][x] = EMPTY;
                player_x = x;
                player_y = y;
                break;
            case PLAYER_ON_GOAL:
                game_grid[y][x] = GOAL;
                player_x = x;
                player_y = y;
                break;
            case BOX_ON_GOAL:
                game_grid[y][x] = BOX_ON_GOAL;
                break;
            case EMPTY:
            default:
                game_grid[y][x] = EMPTY;
                break;
        }
        x++;
    }
    level_height = y + 1;
    if (x > level_width) level_width = x;
}

void init_level()
{
    moves = 0;
    pushes = 0;
    parse_level();
}

void draw_cell(x, y, cell_type)
int x, y, cell_type;
{
    int screen_x, screen_y, cx, cy;

    screen_x = x * CELL_SIZE + 30;
    screen_y = y * CELL_SIZE + 50;
    cx = screen_x + CELL_SIZE / 2;
    cy = screen_y + CELL_SIZE / 2;

    /* Draw floor background first */
    pw_rop(pw, screen_x, screen_y, CELL_SIZE, CELL_SIZE,
           PIX_SRC | PIX_COLOR(COLOR_FLOOR), NULL, 0, 0);

    switch (cell_type) {
        case WALL:
            /* Draw brick pattern */
            pw_rop(pw, screen_x, screen_y, CELL_SIZE, CELL_SIZE,
                   PIX_SRC | PIX_COLOR(COLOR_WALL), NULL, 0, 0);
            /* Add brick lines */
            pw_vector(pw, screen_x, screen_y + CELL_SIZE/3, screen_x + CELL_SIZE, screen_y + CELL_SIZE/3,
                      PIX_SRC | PIX_COLOR(COLOR_BACKGROUND), 1);
            pw_vector(pw, screen_x, screen_y + 2*CELL_SIZE/3, screen_x + CELL_SIZE, screen_y + 2*CELL_SIZE/3,
                      PIX_SRC | PIX_COLOR(COLOR_BACKGROUND), 1);
            pw_vector(pw, screen_x + CELL_SIZE/2, screen_y, screen_x + CELL_SIZE/2, screen_y + CELL_SIZE/3,
                      PIX_SRC | PIX_COLOR(COLOR_BACKGROUND), 1);
            pw_vector(pw, screen_x + CELL_SIZE/4, screen_y + CELL_SIZE/3, screen_x + CELL_SIZE/4, screen_y + 2*CELL_SIZE/3,
                      PIX_SRC | PIX_COLOR(COLOR_BACKGROUND), 1);
            pw_vector(pw, screen_x + 3*CELL_SIZE/4, screen_y + CELL_SIZE/3, screen_x + 3*CELL_SIZE/4, screen_y + 2*CELL_SIZE/3,
                      PIX_SRC | PIX_COLOR(COLOR_BACKGROUND), 1);
            pw_vector(pw, screen_x + CELL_SIZE/2, screen_y + 2*CELL_SIZE/3, screen_x + CELL_SIZE/2, screen_y + CELL_SIZE,
                      PIX_SRC | PIX_COLOR(COLOR_BACKGROUND), 1);
            break;
        case BOX:
            /* Draw crate with wood grain */
            pw_rop(pw, screen_x + 2, screen_y + 2, CELL_SIZE - 4, CELL_SIZE - 4,
                   PIX_SRC | PIX_COLOR(COLOR_BOX), NULL, 0, 0);
            /* Draw crate edges */
            pw_vector(pw, screen_x + 2, screen_y + 2, screen_x + CELL_SIZE - 2, screen_y + 2,
                      PIX_SRC | PIX_COLOR(COLOR_BACKGROUND), 2);
            pw_vector(pw, screen_x + 2, screen_y + 2, screen_x + 2, screen_y + CELL_SIZE - 2,
                      PIX_SRC | PIX_COLOR(COLOR_BACKGROUND), 2);
            pw_vector(pw, screen_x + CELL_SIZE - 2, screen_y + 2, screen_x + CELL_SIZE - 2, screen_y + CELL_SIZE - 2,
                      PIX_SRC | PIX_COLOR(COLOR_BACKGROUND), 2);
            pw_vector(pw, screen_x + 2, screen_y + CELL_SIZE - 2, screen_x + CELL_SIZE - 2, screen_y + CELL_SIZE - 2,
                      PIX_SRC | PIX_COLOR(COLOR_BACKGROUND), 2);
            /* Draw wood grain lines */
            pw_vector(pw, screen_x + 6, screen_y + 6, screen_x + CELL_SIZE - 6, screen_y + 6,
                      PIX_SRC | PIX_COLOR(COLOR_BACKGROUND), 1);
            pw_vector(pw, screen_x + 6, screen_y + CELL_SIZE/2, screen_x + CELL_SIZE - 6, screen_y + CELL_SIZE/2,
                      PIX_SRC | PIX_COLOR(COLOR_BACKGROUND), 1);
            pw_vector(pw, screen_x + 6, screen_y + CELL_SIZE - 6, screen_x + CELL_SIZE - 6, screen_y + CELL_SIZE - 6,
                      PIX_SRC | PIX_COLOR(COLOR_BACKGROUND), 1);
            break;
        case GOAL:
            /* Draw target as crosshairs */
            pw_vector(pw, cx - 8, cy, cx + 8, cy,
                      PIX_SRC | PIX_COLOR(COLOR_TARGET), 2);
            pw_vector(pw, cx, cy - 8, cx, cy + 8,
                      PIX_SRC | PIX_COLOR(COLOR_TARGET), 2);
            /* Draw target rings */
            pw_rop(pw, cx - 6, cy - 6, 12, 12,
                   PIX_SRC | PIX_COLOR(COLOR_TARGET), NULL, 0, 0);
            pw_rop(pw, cx - 3, cy - 3, 6, 6,
                   PIX_SRC | PIX_COLOR(COLOR_FLOOR), NULL, 0, 0);
            break;
        case BOX_ON_GOAL:
            /* Draw target underneath */
            pw_vector(pw, cx - 8, cy, cx + 8, cy,
                      PIX_SRC | PIX_COLOR(COLOR_TARGET), 2);
            pw_vector(pw, cx, cy - 8, cx, cy + 8,
                      PIX_SRC | PIX_COLOR(COLOR_TARGET), 2);
            /* Draw crate on top with different color */
            pw_rop(pw, screen_x + 2, screen_y + 2, CELL_SIZE - 4, CELL_SIZE - 4,
                   PIX_SRC | PIX_COLOR(COLOR_BOX_ON_TARGET), NULL, 0, 0);
            pw_vector(pw, screen_x + 2, screen_y + 2, screen_x + CELL_SIZE - 2, screen_y + 2,
                      PIX_SRC | PIX_COLOR(COLOR_TEXT), 2);
            pw_vector(pw, screen_x + 2, screen_y + 2, screen_x + 2, screen_y + CELL_SIZE - 2,
                      PIX_SRC | PIX_COLOR(COLOR_TEXT), 2);
            pw_vector(pw, screen_x + CELL_SIZE - 2, screen_y + 2, screen_x + CELL_SIZE - 2, screen_y + CELL_SIZE - 2,
                      PIX_SRC | PIX_COLOR(COLOR_TEXT), 2);
            pw_vector(pw, screen_x + 2, screen_y + CELL_SIZE - 2, screen_x + CELL_SIZE - 2, screen_y + CELL_SIZE - 2,
                      PIX_SRC | PIX_COLOR(COLOR_TEXT), 2);
            break;
        case EMPTY:
        default:
            /* Just floor, already drawn above */
            break;
    }
}

void draw_game()
{
    int x, y;
    char status[100];

    /* Clear background */
    pw_rop(pw, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
           PIX_SRC | PIX_COLOR(COLOR_BACKGROUND), NULL, 0, 0);

    /* Draw level */
    for (y = 0; y < level_height; y++) {
        for (x = 0; x < level_width; x++) {
            draw_cell(x, y, game_grid[y][x]);
        }
    }

    /* Draw player as stick figure */
    draw_cell(player_x, player_y, EMPTY);
    {
        int px, py, pcx, pcy;
        px = player_x * CELL_SIZE + 30;
        py = player_y * CELL_SIZE + 50;
        pcx = px + CELL_SIZE / 2;
        pcy = py + CELL_SIZE / 2;

        /* Draw head (circle) */
        pw_rop(pw, pcx - 4, py + 4, 8, 8,
               PIX_SRC | PIX_COLOR(COLOR_PLAYER), NULL, 0, 0);
        pw_vector(pw, pcx - 3, py + 5, pcx + 3, py + 5,
                  PIX_SRC | PIX_COLOR(COLOR_FLOOR), 1);
        pw_vector(pw, pcx - 3, py + 10, pcx + 3, py + 10,
                  PIX_SRC | PIX_COLOR(COLOR_FLOOR), 1);

        /* Draw body */
        pw_vector(pw, pcx, py + 12, pcx, py + CELL_SIZE - 6,
                  PIX_SRC | PIX_COLOR(COLOR_PLAYER), 2);

        /* Draw arms */
        pw_vector(pw, pcx - 6, py + 15, pcx + 6, py + 15,
                  PIX_SRC | PIX_COLOR(COLOR_PLAYER), 2);

        /* Draw legs */
        pw_vector(pw, pcx, py + CELL_SIZE - 6, pcx - 4, py + CELL_SIZE - 2,
                  PIX_SRC | PIX_COLOR(COLOR_PLAYER), 2);
        pw_vector(pw, pcx, py + CELL_SIZE - 6, pcx + 4, py + CELL_SIZE - 2,
                  PIX_SRC | PIX_COLOR(COLOR_PLAYER), 2);
    }

    /* Draw status */
    sprintf(status, "Level: %d/%d  Moves: %d  Pushes: %d",
            current_level + 1, NUM_LEVELS, moves, pushes);
    pw_text(pw, 30, 25, PIX_SRC | PIX_COLOR(COLOR_TEXT), 0, status);

    pw_text(pw, 30, WINDOW_HEIGHT - 20, PIX_SRC | PIX_COLOR(COLOR_TEXT), 0,
            "WASD/hjkl to move, R to restart, N for next level, Q to quit");
}

int can_move(dx, dy)
int dx, dy;
{
    int new_x, new_y, next_x, next_y;
    int cell, next_cell;

    new_x = player_x + dx;
    new_y = player_y + dy;

    if (new_x < 0 || new_x >= level_width || new_y < 0 || new_y >= level_height)
        return 0;

    cell = game_grid[new_y][new_x];

    if (cell == WALL)
        return 0;

    if (cell == BOX || cell == BOX_ON_GOAL) {
        next_x = new_x + dx;
        next_y = new_y + dy;

        if (next_x < 0 || next_x >= level_width || next_y < 0 || next_y >= level_height)
            return 0;

        next_cell = game_grid[next_y][next_x];

        if (next_cell == WALL || next_cell == BOX || next_cell == BOX_ON_GOAL)
            return 0;
    }

    return 1;
}

void make_move(dx, dy)
int dx, dy;
{
    int new_x, new_y, next_x, next_y;
    int cell, next_cell;
    int is_push = 0;

    new_x = player_x + dx;
    new_y = player_y + dy;
    cell = game_grid[new_y][new_x];

    if (cell == BOX || cell == BOX_ON_GOAL) {
        next_x = new_x + dx;
        next_y = new_y + dy;
        next_cell = game_grid[next_y][next_x];

        /* Move the box */
        if (cell == BOX_ON_GOAL) {
            game_grid[new_y][new_x] = GOAL;
        } else {
            game_grid[new_y][new_x] = EMPTY;
        }

        if (next_cell == GOAL) {
            game_grid[next_y][next_x] = BOX_ON_GOAL;
        } else {
            game_grid[next_y][next_x] = BOX;
        }

        is_push = 1;
        pushes++;
    }

    player_x = new_x;
    player_y = new_y;
    moves++;

    if (is_push) {
        check_win();
    }
}

void check_win()
{
    int x, y, boxes_on_goals = 0, total_goals = 0;

    for (y = 0; y < level_height; y++) {
        for (x = 0; x < level_width; x++) {
            if (game_grid[y][x] == GOAL)
                total_goals++;
            else if (game_grid[y][x] == BOX_ON_GOAL) {
                boxes_on_goals++;
                total_goals++;
            }
        }
    }

    if (boxes_on_goals == total_goals && total_goals > 0) {
        /* Level completed */
        if (current_level < NUM_LEVELS - 1) {
            current_level++;
            init_level();
        }
        draw_game();
    }
}

void handle_input(window, event, arg)
Window window;
Event *event;
caddr_t arg;
{
    unsigned short key_id;

    if (event_is_up(event))
        return;

    key_id = event_id(event);

    if (event_is_ascii(event)) {
        switch (key_id) {
            case 'q':
            case 'Q':
                exit(0);
                break;
            case 'r':
            case 'R':
                init_level();
                draw_game();
                break;
            case 'n':
            case 'N':
                if (current_level < NUM_LEVELS - 1) {
                    current_level++;
                    init_level();
                    draw_game();
                }
                break;
            case 'p':
            case 'P':
                if (current_level > 0) {
                    current_level--;
                    init_level();
                    draw_game();
                }
                break;
            case 'w':
            case 'W':
            case 'k':
            case 'K':
                if (can_move(0, -1)) {
                    make_move(0, -1);
                    draw_game();
                }
                break;
            case 's':
            case 'S':
            case 'j':
            case 'J':
                if (can_move(0, 1)) {
                    make_move(0, 1);
                    draw_game();
                }
                break;
            case 'a':
            case 'A':
            case 'h':
            case 'H':
                if (can_move(-1, 0)) {
                    make_move(-1, 0);
                    draw_game();
                }
                break;
            case 'd':
            case 'D':
            case 'l':
            case 'L':
                if (can_move(1, 0)) {
                    make_move(1, 0);
                    draw_game();
                }
                break;
        }
    }
}