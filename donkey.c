/* Donkey.bas clone for sunos4 sunview
   written by claude, public domain
   cc donkey.c -o donkey -lsuntool -lsunwindow -lpixrect */

#include <suntool/sunview.h>
#include <suntool/canvas.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 600
#define NUM_COLORS 16
#define CAR_WIDTH 40
#define CAR_HEIGHT 60
#define DONKEY_WIDTH 60
#define DONKEY_HEIGHT 40
#define ROAD_WIDTH 200
#define LANE_WIDTH 100

#define COLOR_BACKGROUND 0
#define COLOR_CAR 1
#define COLOR_DONKEY 2
#define COLOR_TEXT 3
#define COLOR_ROAD 4
#define COLOR_GRASS 5
#define COLOR_TERM 6

Frame frame;
Canvas canvas;
Pixwin *pw;

int car_lane;
int donkey_y;
int donkey_lane;
int player_score;
int donkey_score;
int game_over;
int cms_size;
unsigned char red[NUM_COLORS], green[NUM_COLORS], blue[NUM_COLORS];

/* Function prototypes */
main();
setup_colors();
init_game();
draw_game();
update_game();
handle_input();
Notify_value game_timer();
draw_car();
draw_donkey();

main(argc, argv)
int argc;
char **argv;
{
    struct itimerval timer_value;

    srand(time(0));
    
    frame = window_create(NULL, FRAME,
                          FRAME_LABEL, "Donkey.bas SunView",
                          WIN_WIDTH, WINDOW_WIDTH,
                          WIN_HEIGHT, WINDOW_HEIGHT,
                          0);
    if (frame == NULL) {
        fprintf(stderr, "Failed to create frame\n");
        exit(1);
    }

    canvas = window_create(frame, CANVAS,
                           WIN_WIDTH, WINDOW_WIDTH,
                           WIN_HEIGHT, WINDOW_HEIGHT,
                           WIN_EVENT_PROC, handle_input,
                           WIN_CONSUME_KBD_EVENTS, WIN_UP_EVENTS | WIN_ASCII_EVENTS,
                           0);
    if (canvas == NULL) {
        fprintf(stderr, "Failed to create canvas\n");
        exit(1);
    }

    pw = canvas_pixwin(canvas);
    if (pw == NULL) {
        fprintf(stderr, "Failed to get pixwin\n");
        exit(1);
    }

    setup_colors();
    init_game();
    window_fit(frame);

    /* Set up the timer */
    timer_value.it_value.tv_sec = 0;
    timer_value.it_value.tv_usec = 50000; /* 20 FPS */
    timer_value.it_interval = timer_value.it_value;
    notify_set_itimer_func(frame, game_timer, ITIMER_REAL, &timer_value, NULL);

    window_main_loop(frame);
    exit(0);
}

setup_colors()
{
    cms_size = NUM_COLORS;
    red[COLOR_BACKGROUND] = 0; green[COLOR_BACKGROUND] = 0; blue[COLOR_BACKGROUND] = 0;
    red[COLOR_CAR] = 255; green[COLOR_CAR] = 0; blue[COLOR_CAR] = 0;
    red[COLOR_DONKEY] = 255; green[COLOR_DONKEY] = 255; blue[COLOR_DONKEY] = 255;
    red[COLOR_TEXT] = 0; green[COLOR_TEXT] = 0; blue[COLOR_TEXT] = 0;
    red[COLOR_ROAD] = 50; green[COLOR_ROAD] = 50; blue[COLOR_ROAD] = 50;
    red[COLOR_GRASS] = 0; green[COLOR_GRASS] = 255; blue[COLOR_GRASS] = 0;
    red[COLOR_TERM] = 0; green[COLOR_TERM] = 0; blue[COLOR_TERM] = 0;
    
    pw_setcmsname(pw, "donkey_cms");
    pw_putcolormap(pw, 0, NUM_COLORS, red, green, blue);
}

init_game()
{
    car_lane = 0;
    donkey_y = -DONKEY_HEIGHT;
    donkey_lane = rand() % 2;
    player_score = 0;
    donkey_score = 0;
    game_over = 0;
}

draw_car(x, y)
int x, y;
{
    /* Car body */
    pw_rop(pw, x, y + 20, CAR_WIDTH, CAR_HEIGHT - 20, PIX_SRC | PIX_COLOR(COLOR_CAR), 0, 0, 0);
    
    /* Car roof */
    pw_rop(pw, x + 5, y, CAR_WIDTH - 10, 20, PIX_SRC | PIX_COLOR(COLOR_CAR), 0, 0, 0);
    
    /* Windows */
    pw_rop(pw, x + 7, y + 5, 10, 15, PIX_SRC | PIX_COLOR(COLOR_TEXT), 0, 0, 0);
    pw_rop(pw, x + CAR_WIDTH - 17, y + 5, 10, 15, PIX_SRC | PIX_COLOR(COLOR_TEXT), 0, 0, 0);
    
    /* Wheels */
    pw_rop(pw, x + 5, y + CAR_HEIGHT - 10, 10, 10, PIX_SRC | PIX_COLOR(COLOR_TEXT), 0, 0, 0);
    pw_rop(pw, x + CAR_WIDTH - 15, y + CAR_HEIGHT - 10, 10, 10, PIX_SRC | PIX_COLOR(COLOR_TEXT), 0, 0, 0);
}

draw_donkey(x, y)
int x, y;
{
    /* Donkey body */
    pw_rop(pw, x + 10, y + 10, DONKEY_WIDTH - 20, DONKEY_HEIGHT - 10, PIX_SRC | PIX_COLOR(COLOR_DONKEY), 0, 0, 0);
    
    /* Head */
    pw_rop(pw, x, y, 20, 20, PIX_SRC | PIX_COLOR(COLOR_DONKEY), 0, 0, 0);
    
    /* Ears */
    pw_vector(pw, x + 5, y, x, y - 10, PIX_SRC | PIX_COLOR(COLOR_DONKEY), 2);
    pw_vector(pw, x + 15, y, x + 20, y - 10, PIX_SRC | PIX_COLOR(COLOR_DONKEY), 2);
    
    /* Legs */
    pw_vector(pw, x + 15, y + DONKEY_HEIGHT - 10, x + 15, y + DONKEY_HEIGHT, PIX_SRC | PIX_COLOR(COLOR_DONKEY), 2);
    pw_vector(pw, x + 35, y + DONKEY_HEIGHT - 10, x + 35, y + DONKEY_HEIGHT, PIX_SRC | PIX_COLOR(COLOR_DONKEY), 2);
    
    /* Tail */
    pw_vector(pw, x + DONKEY_WIDTH - 10, y + 15, x + DONKEY_WIDTH, y + 5, PIX_SRC | PIX_COLOR(COLOR_DONKEY), 2);
}

draw_game()
{
    char score_str[40];
    int road_left = (WINDOW_WIDTH - ROAD_WIDTH) / 2;
    int road_right = road_left + ROAD_WIDTH;
    int i;

    /* Clear background */
    pw_writebackground(pw, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, PIX_SRC | PIX_COLOR(COLOR_BACKGROUND));

    /* Draw grass */
    pw_rop(pw, 0, 0, road_left, WINDOW_HEIGHT, PIX_SRC | PIX_COLOR(COLOR_GRASS), 0, 0, 0);
    pw_rop(pw, road_right, 0, road_left, WINDOW_HEIGHT, PIX_SRC | PIX_COLOR(COLOR_GRASS), 0, 0, 0);

    /* Draw road */
    pw_rop(pw, road_left, 0, ROAD_WIDTH, WINDOW_HEIGHT, PIX_SRC | PIX_COLOR(COLOR_ROAD), 0, 0, 0);

    /* Draw road markings */
    pw_vector(pw, road_left, 0, road_left, WINDOW_HEIGHT, PIX_SRC | PIX_COLOR(COLOR_TEXT), 1);
    pw_vector(pw, road_right, 0, road_right, WINDOW_HEIGHT, PIX_SRC | PIX_COLOR(COLOR_TEXT), 1);
    
    /* Draw dashed center line */
    for (i = 0; i < WINDOW_HEIGHT; i += 40) {
        pw_vector(pw, WINDOW_WIDTH/2, i, WINDOW_WIDTH/2, i + 20, PIX_SRC | PIX_COLOR(COLOR_DONKEY), 1);
    }

    /* Draw car */
    draw_car(road_left + car_lane * LANE_WIDTH + (LANE_WIDTH - CAR_WIDTH)/2, WINDOW_HEIGHT - CAR_HEIGHT - 10);

    /* Draw donkey */
    draw_donkey(road_left + donkey_lane * LANE_WIDTH + (LANE_WIDTH - DONKEY_WIDTH)/2, donkey_y);

    /* Draw score */
    sprintf(score_str, "Player: %d  Donkey: %d", player_score, donkey_score);
    pw_text(pw, 10, 20, PIX_SRC | PIX_COLOR(COLOR_TEXT), 0, score_str);

    /* Draw instructions */
    pw_text(pw, 10, WINDOW_HEIGHT - 20, PIX_SRC | PIX_COLOR(COLOR_TEXT), 0, "Space: Change Lane  Q: Quit");

    if (game_over) {
        pw_text(pw, WINDOW_WIDTH/2 - 30, WINDOW_HEIGHT/2, PIX_SRC | PIX_COLOR(COLOR_TEXT), 0, "BOOM!");
    }
}

update_game()
{
    if (game_over) {
        init_game();
        return;
    }

    /* Move donkey */
    donkey_y += 5;

    /* Check for collision */
    if (donkey_y + DONKEY_HEIGHT > WINDOW_HEIGHT - CAR_HEIGHT - 10 && car_lane == donkey_lane) {
        game_over = 1;
        donkey_score++;
    }

    /* Check if donkey passed */
    if (donkey_y > WINDOW_HEIGHT) {
        donkey_y = -DONKEY_HEIGHT;
        donkey_lane = rand() % 2;
        player_score++;
    }

    draw_game();
}

handle_input(window, event, arg)
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
            case ' ':
                if (!game_over) {
                    car_lane = 1 - car_lane;  /* Switch lanes */
                }
                break;
        }
    }
}

Notify_value
game_timer(client, itimer_type)
    Notify_client client;
    int itimer_type;
{
    update_game();
    return NOTIFY_DONE;
}
