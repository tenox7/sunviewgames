/* Flappy Bird clone for sunos4 sunview
   written by claude, public domain
   cc flappybird.c -o flappybird -lsuntool -lsunwindow -lpixrect */

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
#define BIRD_WIDTH 40
#define BIRD_HEIGHT 30
#define PIPE_WIDTH 60
#define GAP_HEIGHT 150
#define NUM_COLORS 8

#define COLOR_BACKGROUND 0
#define COLOR_BIRD 1
#define COLOR_PIPE 2
#define COLOR_TEXT 3

Frame frame;
Canvas canvas;
Pixwin *pw;

int bird_y;
float bird_velocity;
int pipe_x;
int pipe_gap_y;
int score;
int game_over;
int cms_size;
int flap_state;
unsigned char red[NUM_COLORS], green[NUM_COLORS], blue[NUM_COLORS];

/* Function prototypes */
setup_colors();
init_game();
draw_game();
update_game();
handle_input();
Notify_value game_timer();
main();

setup_colors()
{
    cms_size = NUM_COLORS;
    red[COLOR_BACKGROUND] = 135; green[COLOR_BACKGROUND] = 206; blue[COLOR_BACKGROUND] = 235;
    red[COLOR_BIRD] = 255; green[COLOR_BIRD] = 255; blue[COLOR_BIRD] = 0;
    red[COLOR_PIPE] = 0; green[COLOR_PIPE] = 128; blue[COLOR_PIPE] = 0;
    red[COLOR_TEXT] = 255; green[COLOR_TEXT] = 255; blue[COLOR_TEXT] = 255;
    
    pw_setcmsname(pw, "flappybird_cms");
    pw_putcolormap(pw, 0, NUM_COLORS, red, green, blue);
}

init_game()
{
    bird_y = WINDOW_HEIGHT / 2;
    bird_velocity = 0;
    pipe_x = WINDOW_WIDTH;
    pipe_gap_y = WINDOW_HEIGHT / 2;
    score = 0;
    game_over = 0;
    flap_state = 0;
}

draw_game()
{
    char score_str[20];
    int bird_x = WINDOW_WIDTH/4;

    /* Clear background */
    pw_writebackground(pw, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, PIX_SRC | PIX_COLOR(COLOR_BACKGROUND));

    /* Draw bird (old Twitter logo style) */
    /* Body */
    pw_vector(pw, bird_x - BIRD_WIDTH/2, bird_y, 
              bird_x + BIRD_WIDTH/2, bird_y, PIX_SRC | PIX_COLOR(COLOR_BIRD), 2);
    
    /* Head */
    pw_vector(pw, bird_x + BIRD_WIDTH/4, bird_y - BIRD_HEIGHT/4, 
              bird_x + BIRD_WIDTH/2, bird_y - BIRD_HEIGHT/2, PIX_SRC | PIX_COLOR(COLOR_BIRD), 2);
    pw_vector(pw, bird_x + BIRD_WIDTH/2, bird_y - BIRD_HEIGHT/2, 
              bird_x + BIRD_WIDTH/2 + BIRD_WIDTH/8, bird_y - BIRD_HEIGHT/4, PIX_SRC | PIX_COLOR(COLOR_BIRD), 2);
    
    /* Beak */
    pw_vector(pw, bird_x + BIRD_WIDTH/2 + BIRD_WIDTH/8, bird_y - BIRD_HEIGHT/4, 
              bird_x + BIRD_WIDTH/2 + BIRD_WIDTH/4, bird_y - BIRD_HEIGHT/8, PIX_SRC | PIX_COLOR(COLOR_BIRD), 1);
    
    /* Tail */
    pw_vector(pw, bird_x - BIRD_WIDTH/2, bird_y, 
              bird_x - BIRD_WIDTH/2 - BIRD_WIDTH/4, bird_y + BIRD_HEIGHT/4, PIX_SRC | PIX_COLOR(COLOR_BIRD), 2);

    /* Wings (flapping) */
    if (flap_state) {
        pw_vector(pw, bird_x - BIRD_WIDTH/4, bird_y, 
                  bird_x, bird_y - BIRD_HEIGHT/2, PIX_SRC | PIX_COLOR(COLOR_BIRD), 2);
    } else {
        pw_vector(pw, bird_x - BIRD_WIDTH/4, bird_y, 
                  bird_x, bird_y + BIRD_HEIGHT/3, PIX_SRC | PIX_COLOR(COLOR_BIRD), 2);
    }

    /* Draw pipes */
    pw_rop(pw, pipe_x, 0, PIPE_WIDTH, pipe_gap_y - GAP_HEIGHT/2, 
           PIX_SRC | PIX_COLOR(COLOR_PIPE), 0, 0, 0);
    pw_rop(pw, pipe_x, pipe_gap_y + GAP_HEIGHT/2, PIPE_WIDTH, WINDOW_HEIGHT - (pipe_gap_y + GAP_HEIGHT/2), 
           PIX_SRC | PIX_COLOR(COLOR_PIPE), 0, 0, 0);

    /* Draw score */
    sprintf(score_str, "Score: %d", score);
    pw_text(pw, 10, 30, PIX_SRC | PIX_COLOR(COLOR_TEXT), 0, score_str);

    if (game_over) {
        pw_text(pw, WINDOW_WIDTH/2 - 40, WINDOW_HEIGHT/2, PIX_SRC | PIX_COLOR(COLOR_TEXT), 0, "Game Over!");
        pw_text(pw, WINDOW_WIDTH/2 - 80, WINDOW_HEIGHT/2 + 30, PIX_SRC | PIX_COLOR(COLOR_TEXT), 0, "Press SPACE to restart");
    }
}

update_game()
{
    if (game_over) return;

    /* Update bird position */
    bird_velocity += 0.5;
    bird_y += bird_velocity;

    /* Update pipe position */
    pipe_x -= 2;

    /* Check for collision or out of bounds */
    if (bird_y - BIRD_HEIGHT/2 < 0 || bird_y + BIRD_HEIGHT/2 > WINDOW_HEIGHT ||
        (pipe_x < WINDOW_WIDTH/4 + BIRD_WIDTH/2 && pipe_x > WINDOW_WIDTH/4 - PIPE_WIDTH - BIRD_WIDTH/2 &&
         (bird_y - BIRD_HEIGHT/2 < pipe_gap_y - GAP_HEIGHT/2 || bird_y + BIRD_HEIGHT/2 > pipe_gap_y + GAP_HEIGHT/2))) {
        game_over = 1;
    }

    /* Check if pipe has passed */
    if (pipe_x < 0) {
        pipe_x = WINDOW_WIDTH;
        pipe_gap_y = rand() % (WINDOW_HEIGHT - GAP_HEIGHT) + GAP_HEIGHT/2;
        score++;
    }

    /* Update flap state */
    flap_state = !flap_state;

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
                if (game_over) {
                    init_game();
                } else {
                    bird_velocity = -8;
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

main(argc, argv)
int argc;
char **argv;
{
    struct itimerval timer_value;

    srand(time(0));
    
    frame = window_create(NULL, FRAME,
                          FRAME_LABEL, "Flappy Bird SunView",
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
    timer_value.it_value.tv_usec = 33333; /* Approx. 30 FPS */
    timer_value.it_interval = timer_value.it_value;
    notify_set_itimer_func(frame, game_timer, ITIMER_REAL, &timer_value, NULL);

    window_main_loop(frame);
    exit(0);
}
