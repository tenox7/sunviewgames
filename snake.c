/* sunview snake game, written by claude, public domain
   cc snake.c -o snake -lsuntool -lsunwindow -lpixrect */
#include <suntool/sunview.h>
#include <suntool/canvas.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>

#define WINDOW_WIDTH 500
#define WINDOW_HEIGHT 500
#define GRID_SIZE 25
#define BLOCK_SIZE 20
#define GAME_SPEED 300000  /* microseconds between moves */

#define UP    0
#define DOWN  1
#define LEFT  2
#define RIGHT 3

#define COLOR_BLACK   0
#define COLOR_GREEN   1
#define COLOR_RED     2
#define COLOR_WHITE   3
#define COLOR_YELLOW  4
#define COLOR_BLUE    5

Frame frame;
Canvas canvas;
Pixwin *pw;

typedef struct {
    int x, y;
} Point;

Point snake[400];
int snake_length;
int direction;
Point food;
int score;
int game_over;

void init_game();
void draw_game();
void move_snake();
void generate_food();
void check_collision();
void handle_input();
void draw_block();
Notify_value game_tick();

main(argc, argv)
int argc;
char **argv;
{
    struct itimerval timer;
    unsigned char red[8], green[8], blue[8];

    srand(time(0));
    frame = window_create(NULL, FRAME,
        FRAME_LABEL,       "Snake SunView",
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

    /* Set up bright, distinct colors */
    red[COLOR_BLACK] = 0;     green[COLOR_BLACK] = 0;     blue[COLOR_BLACK] = 0;     /* Black */
    red[COLOR_GREEN] = 0;     green[COLOR_GREEN] = 255;   blue[COLOR_GREEN] = 0;     /* Bright Green */
    red[COLOR_RED] = 255;     green[COLOR_RED] = 0;       blue[COLOR_RED] = 0;       /* Bright Red */
    red[COLOR_WHITE] = 255;   green[COLOR_WHITE] = 255;   blue[COLOR_WHITE] = 255;   /* White */
    red[COLOR_YELLOW] = 255;  green[COLOR_YELLOW] = 255;  blue[COLOR_YELLOW] = 0;    /* Bright Yellow */
    red[COLOR_BLUE] = 0;      green[COLOR_BLUE] = 0;      blue[COLOR_BLUE] = 255;    /* Bright Blue */

    pw_setcmsname(pw, "snake_colormap");
    pw_putcolormap(pw, 0, 8, red, green, blue);

    window_fit(frame);

    init_game();
    draw_game();

    /* Set up the timer */
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = GAME_SPEED;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = GAME_SPEED;
    notify_set_itimer_func(frame, game_tick, ITIMER_REAL, &timer, NULL);

    window_main_loop(frame);
    exit(0);
}

void init_game()
{
    snake_length = 3;
    snake[0].x = GRID_SIZE / 2;
    snake[0].y = GRID_SIZE / 2;
    snake[1].x = GRID_SIZE / 2 - 1;
    snake[1].y = GRID_SIZE / 2;
    snake[2].x = GRID_SIZE / 2 - 2;
    snake[2].y = GRID_SIZE / 2;

    direction = RIGHT;
    score = 0;
    game_over = 0;
    generate_food();
}

void draw_block(x, y, color)
int x, y, color;
{
    pw_rop(pw, x * BLOCK_SIZE + 1, y * BLOCK_SIZE + 1,
           BLOCK_SIZE - 2, BLOCK_SIZE - 2,
           PIX_SRC | PIX_COLOR(color), NULL, 0, 0);
}

void draw_food_special(x, y)
int x, y;
{
    int cx = x * BLOCK_SIZE + BLOCK_SIZE/2;
    int cy = y * BLOCK_SIZE + BLOCK_SIZE/2;
    int radius = BLOCK_SIZE/3;
    
    /* Draw bright red filled rectangle for food */
    pw_rop(pw, x * BLOCK_SIZE + 1, y * BLOCK_SIZE + 1,
           BLOCK_SIZE - 2, BLOCK_SIZE - 2,
           PIX_SRC | PIX_COLOR(COLOR_RED), NULL, 0, 0);
           
    /* Add yellow cross to make it look like an apple */
    pw_vector(pw, cx - radius/2, cy, cx + radius/2, cy, 
              PIX_SRC | PIX_COLOR(COLOR_YELLOW), 2);
    pw_vector(pw, cx, cy - radius/2, cx, cy + radius/2, 
              PIX_SRC | PIX_COLOR(COLOR_YELLOW), 2);
}

void draw_game()
{
    int i;
    char str[50];

    /* Clear background to black */
    pw_rop(pw, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
           PIX_SRC | PIX_COLOR(COLOR_BLACK), NULL, 0, 0);

    /* Draw white grid lines */
    for (i = 0; i <= GRID_SIZE; i++) {
        pw_vector(pw, i * BLOCK_SIZE, 0, i * BLOCK_SIZE, GRID_SIZE * BLOCK_SIZE,
                  PIX_SRC | PIX_COLOR(COLOR_WHITE), 1);
        pw_vector(pw, 0, i * BLOCK_SIZE, GRID_SIZE * BLOCK_SIZE, i * BLOCK_SIZE,
                  PIX_SRC | PIX_COLOR(COLOR_WHITE), 1);
    }

    if (!game_over) {
        /* Draw colorful snake */
        for (i = 0; i < snake_length; i++) {
            if (i == 0) {
                /* Yellow head */
                draw_block(snake[i].x, snake[i].y, COLOR_YELLOW);
            } else {
                /* Green body */
                draw_block(snake[i].x, snake[i].y, COLOR_GREEN);
            }
        }

        /* Draw red food with yellow cross */
        draw_food_special(food.x, food.y);
    }

    /* Draw white score text */
    sprintf(str, "Score: %d", score);
    pw_text(pw, 10, WINDOW_HEIGHT - 30, PIX_SRC | PIX_COLOR(COLOR_WHITE), 0, str);

    if (game_over) {
        pw_text(pw, WINDOW_WIDTH/2 - 50, WINDOW_HEIGHT/2, PIX_SRC | PIX_COLOR(COLOR_RED), 0, "Game Over!");
        sprintf(str, "Final Score: %d", score);
        pw_text(pw, WINDOW_WIDTH/2 - 60, WINDOW_HEIGHT/2 + 20, PIX_SRC | PIX_COLOR(COLOR_WHITE), 0, str);
        pw_text(pw, WINDOW_WIDTH/2 - 80, WINDOW_HEIGHT/2 + 40, PIX_SRC | PIX_COLOR(COLOR_WHITE), 0, "Press 'r' to restart");
    }
}

void generate_food()
{
    int valid = 0;
    int i;

    while (!valid) {
        /* Generate food away from walls and corners */
        /* Keep food at least 2 blocks away from any edge */
        food.x = 2 + (rand() % (GRID_SIZE - 4));
        food.y = 2 + (rand() % (GRID_SIZE - 4));

        valid = 1;
        
        /* Check if food overlaps with snake */
        for (i = 0; i < snake_length; i++) {
            if (snake[i].x == food.x && snake[i].y == food.y) {
                valid = 0;
                break;
            }
        }
        
        /* Additional check: avoid corners and edges completely */
        if (valid) {
            /* Avoid corners (within 2 blocks of any corner) */
            if ((food.x <= 2 && food.y <= 2) ||                    /* top-left */
                (food.x >= GRID_SIZE-3 && food.y <= 2) ||          /* top-right */
                (food.x <= 2 && food.y >= GRID_SIZE-3) ||          /* bottom-left */
                (food.x >= GRID_SIZE-3 && food.y >= GRID_SIZE-3))  /* bottom-right */
            {
                valid = 0;
            }
        }
    }
}

void move_snake()
{
    int i;
    Point new_head;

    new_head.x = snake[0].x;
    new_head.y = snake[0].y;

    switch (direction) {
        case UP:    new_head.y--; break;
        case DOWN:  new_head.y++; break;
        case LEFT:  new_head.x--; break;
        case RIGHT: new_head.x++; break;
    }

    /* Check wall collision */
    if (new_head.x < 0 || new_head.x >= GRID_SIZE ||
        new_head.y < 0 || new_head.y >= GRID_SIZE) {
        game_over = 1;
        return;
    }

    /* Check self collision */
    for (i = 0; i < snake_length; i++) {
        if (snake[i].x == new_head.x && snake[i].y == new_head.y) {
            game_over = 1;
            return;
        }
    }

    /* Move snake body */
    for (i = snake_length - 1; i > 0; i--) {
        snake[i] = snake[i-1];
    }
    snake[0] = new_head;

    /* Check food collision */
    if (snake[0].x == food.x && snake[0].y == food.y) {
        snake_length++;
        score += 10;
        generate_food();
    }
}

void handle_input(window, event, arg)
Window window;
Event *event;
caddr_t arg;
{
    unsigned short key_id;

    key_id = event_id(event);

    if (event_is_ascii(event)) {
        if (game_over) {
            if (key_id == 'r' || key_id == 'R') {
                init_game();
                draw_game();
            }
            return;
        }

        switch (key_id) {
            case 'q':
            case 'Q':
                exit(0);
                break;
            case 'w':
            case 'W':
            case 'k':
            case 'K':
                if (direction != DOWN) direction = UP;
                break;
            case 's':
            case 'S':
            case 'j':
            case 'J':
                if (direction != UP) direction = DOWN;
                break;
            case 'a':
            case 'A':
            case 'h':
            case 'H':
                if (direction != RIGHT) direction = LEFT;
                break;
            case 'd':
            case 'D':
            case 'l':
            case 'L':
                if (direction != LEFT) direction = RIGHT;
                break;
        }
    }
}

Notify_value game_tick(client, which)
    Notify_client client;
    int which;
{
    if (!game_over) {
        move_snake();
    }
    draw_game();
    return NOTIFY_DONE;
}