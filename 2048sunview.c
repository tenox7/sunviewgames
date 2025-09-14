/* 2048 game clone for sunos4 sunview
   written by claude, public domain
   cc 2048.c -o 2048 -lsuntool -lsunwindow -lpixrect */
#include <suntool/sunview.h>
#include <suntool/canvas.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 400
#define GRID_SIZE 4
#define NUM_COLORS 32

#define TILE_BACKGROUND 0
#define TILE_2     1
#define TILE_4     2
#define TILE_8     3
#define TILE_16    4
#define TILE_32    5
#define TILE_64    6
#define TILE_128   7
#define TILE_256   8
#define TILE_512   9
#define TILE_1024 10
#define TILE_2048 11
#define TILE_TERM 12

#define TEXT_COLOR TILE_BACKGROUND

Frame frame;
Canvas canvas;
Pixwin *pw;

int grid[GRID_SIZE][GRID_SIZE];
int score;
int cms_size;
unsigned char red[NUM_COLORS], green[NUM_COLORS], blue[NUM_COLORS];

/* Function prototypes */
main();
init_grid();
setup_colors();
get_tile_color();
draw_grid();
add_tile();
move();
game_over();
handle_input();
demo_mode();

main(argc, argv)
int argc;
char **argv;
{
    int demo;
    
    demo = 0;
    if (argc > 1 && strcmp(argv[1], "-demo") == 0) {
        demo = 1;
    }

    srand(time(0));
    
    frame = window_create(NULL, FRAME, 0);
    if (frame == NULL) {
        fprintf(stderr, "Failed to create frame\n");
        exit(1);
    }
    
    window_set(frame, 
               FRAME_LABEL, "2048 SunView", 
               WIN_WIDTH, WINDOW_WIDTH, 
               WIN_HEIGHT, WINDOW_HEIGHT, 
               0);

    canvas = window_create(frame, CANVAS, 0);
    if (canvas == NULL) {
        fprintf(stderr, "Failed to create canvas\n");
        exit(1);
    }
    
    window_set(canvas, 
               WIN_WIDTH, WINDOW_WIDTH, 
               WIN_HEIGHT, WINDOW_HEIGHT, 
               WIN_EVENT_PROC, handle_input,
               WIN_CONSUME_KBD_EVENTS, WIN_UP_EVENTS | WIN_ASCII_EVENTS,
               0);

    pw = canvas_pixwin(canvas);
    if (pw == NULL) {
        fprintf(stderr, "Failed to get pixwin\n");
        exit(1);
    }

    setup_colors();
    window_fit(frame);

    if (demo) {
        demo_mode();
    } else {
        init_grid();
        add_tile();
        add_tile();
        draw_grid();
    }

    window_main_loop(frame);
    exit(0);
}

init_grid()
{
    int i, j;
    
    for (i = 0; i < GRID_SIZE; i++) {
        for (j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = 0;
        }
    }
    score = 0;
}


setup_colors()
{
    cms_size = NUM_COLORS;
    red[TILE_BACKGROUND] = 255;  green[TILE_BACKGROUND] = 255;  blue[TILE_BACKGROUND] = 255;
        
    red[TILE_2]          = 204;  green[TILE_2]          = 192;  blue[TILE_2]          = 180;
    red[TILE_4]          = 237;  green[TILE_4]          = 224;  blue[TILE_4]          = 200;
    red[TILE_8]          = 242;  green[TILE_8]          = 177;  blue[TILE_8]          = 121;
    red[TILE_16]         = 245;  green[TILE_16]         = 149;  blue[TILE_16]         = 99;
    
    red[TILE_32]         = 246;  green[TILE_32]         = 124;  blue[TILE_32]         = 95;
    red[TILE_64]         = 246;  green[TILE_64]         = 94;   blue[TILE_64]         = 59;    
    red[TILE_128]        = 255;  green[TILE_128]        = 64;  blue[TILE_128]        = 0;
    red[TILE_256]        = 255;  green[TILE_256]        = 128;  blue[TILE_256]        = 240;
    
    red[TILE_512]        = 255;  green[TILE_512]        = 0;  blue[TILE_512]        = 255;
    red[TILE_1024]       = 192;  green[TILE_1024]       = 64;  blue[TILE_1024]       = 255;
    red[TILE_2048]       = 128;  green[TILE_2048]       = 0;  blue[TILE_2048]       = 240;
    red[TILE_TERM]       = 255;  green[TILE_TERM]       = 255;  blue[TILE_TERM]       = 255;
    
    pw_setcmsname(pw, "2048_cms");
    pw_putcolormap(pw, 0, NUM_COLORS, red, green, blue);
}


get_tile_color(value)
int value;
{
    switch(value) {
        case 2:    return TILE_2;
        case 4:    return TILE_4;
        case 8:    return TILE_8;
        case 16:   return TILE_16;
        case 32:   return TILE_32;
        case 64:   return TILE_64;
        case 128:  return TILE_128;
        case 256:  return TILE_256;
        case 512:  return TILE_512;
        case 1024: return TILE_1024;
        case 2048: return TILE_2048;
        default:   return TILE_TERM;  /* Use darkest color for values above 2048 */
    }
}

draw_grid()
{
    int i, j;
    char str[20];
    int value, color;

    pw_writebackground(pw, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, PIX_SRC | PIX_COLOR(TILE_BACKGROUND));

    for (i = 0; i < GRID_SIZE; i++) {
        for (j = 0; j < GRID_SIZE; j++) {
            value = grid[i][j];
            color = get_tile_color(value);
            
            pw_rop(pw, j*100+1, i*100+1, 98, 98, PIX_SRC | PIX_COLOR(color), 0, 0, 0);
            pw_vector(pw, j*100, i*100, j*100+100, i*100, PIX_SRC, 1);
            pw_vector(pw, j*100, i*100, j*100, i*100+100, PIX_SRC, 1);
            pw_vector(pw, j*100+100, i*100, j*100+100, i*100+100, PIX_SRC, 1);
            pw_vector(pw, j*100, i*100+100, j*100+100, i*100+100, PIX_SRC, 1);

            if (value != 0) {
                sprintf(str, "%d", value);
                pw_text(pw, j*100+50-strlen(str)*3, i*100+50, PIX_SRC | PIX_COLOR(TEXT_COLOR), 0, str);
            }
        }
    }

    sprintf(str, "Score: %d", score);
    pw_text(pw, 10, WINDOW_HEIGHT - 10, PIX_SRC | PIX_COLOR(TEXT_COLOR), 0, str);
    
    pw_text(pw, 10, WINDOW_HEIGHT - 30, PIX_SRC | PIX_COLOR(TEXT_COLOR), 0, "Use vi keys (hjkl) to move, 'q' to quit");
}

add_tile()
{
    int i, j, r, empty_spots;

    empty_spots = 0;
    for (i = 0; i < GRID_SIZE; i++) {
        for (j = 0; j < GRID_SIZE; j++) {
            if (grid[i][j] == 0) empty_spots++;
        }
    }

    if (empty_spots == 0) return;

    r = rand() % empty_spots;
    for (i = 0; i < GRID_SIZE; i++) {
        for (j = 0; j < GRID_SIZE; j++) {
            if (grid[i][j] != 0) continue;
            if (r == 0) {
                grid[i][j] = (rand() % 10 == 0) ? 4 : 2;
                return;
            }
            r--;
        }
    }
}

move(dx, dy)
int dx, dy;
{
    int i, j, new_i, new_j, value, moved;
    int start, end, step;

    moved = 0;
    start = (dx == 1 || dy == 1) ? GRID_SIZE - 1 : 0;
    end = (dx == 1 || dy == 1) ? -1 : GRID_SIZE;
    step = (dx == 1 || dy == 1) ? -1 : 1;

    for (i = start; i != end; i += step) {
        for (j = start; j != end; j += step) {
            if (grid[i][j] == 0) continue;

            new_i = i;
            new_j = j;
            value = grid[i][j];
            grid[i][j] = 0;

            while (1) {
                new_i += dy;
                new_j += dx;

                if (new_i < 0 || new_i >= GRID_SIZE || new_j < 0 || new_j >= GRID_SIZE) {
                    grid[new_i-dy][new_j-dx] = value;
                    break;
                }

                if (grid[new_i][new_j] == 0) {
                    moved = 1;
                    continue;
                }

                if (grid[new_i][new_j] == value) {
                    grid[new_i][new_j] *= 2;
                    score += grid[new_i][new_j];
                    moved = 1;
                    break;
                }

                grid[new_i-dy][new_j-dx] = value;
                break;
            }
        }
    }

    return moved;
}

game_over()
{
    int i, j;

    for (i = 0; i < GRID_SIZE; i++) {
        for (j = 0; j < GRID_SIZE; j++) {
            if (grid[i][j] == 0) return 0;
            if (i > 0 && grid[i][j] == grid[i-1][j]) return 0;
            if (i < GRID_SIZE-1 && grid[i][j] == grid[i+1][j]) return 0;
            if (j > 0 && grid[i][j] == grid[i][j-1]) return 0;
            if (j < GRID_SIZE-1 && grid[i][j] == grid[i][j+1]) return 0;
        }
    }
    return 1;
}

handle_input(window, event, arg)
Window window;
Event *event;
caddr_t arg;
{
    int moved;
    unsigned short key_id;

    moved = 0;
    key_id = event_id(event);

    if (event_is_ascii(event)) {
        switch (key_id) {
            case 'q':
            case 'Q':
                exit(0);
                break;
            case 'h':
            case 'H':
                moved = move(-1, 0);  /* Left */
                break;
            case 'l':
            case 'L':
                moved = move(1, 0);   /* Right */
                break;
            case 'k':
            case 'K':
                moved = move(0, -1);  /* Up */
                break;
            case 'j':
            case 'J':
                moved = move(0, 1);   /* Down */
                break;
        }
    }

    if (moved) {
        add_tile();
        draw_grid();
        if (game_over()) {
            pw_text(pw, 150, 200, PIX_SRC | PIX_COLOR(TEXT_COLOR), 0, "Game Over!");
        }
    }
}

demo_mode()
{
    int i, j, value, color;
    char str[20];

    pw_writebackground(pw, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, PIX_SRC | PIX_COLOR(TILE_BACKGROUND));

    value = 2;
    for (i = 0; i < GRID_SIZE; i++) {
        for (j = 0; j < GRID_SIZE; j++) {
            color = get_tile_color(value);
            pw_rop(pw, j*100+1, i*100+1, 98, 98, PIX_SRC | PIX_COLOR(color), 0, 0, 0);
            pw_vector(pw, j*100, i*100, j*100+100, i*100, PIX_SRC, 1);
            pw_vector(pw, j*100, i*100, j*100, i*100+100, PIX_SRC, 1);
            pw_vector(pw, j*100+100, i*100, j*100+100, i*100+100, PIX_SRC, 1);
            pw_vector(pw, j*100, i*100+100, j*100+100, i*100+100, PIX_SRC, 1);
            sprintf(str, "%d", value);
            pw_text(pw, j*100+50-strlen(str)*3, i*100+50, PIX_SRC | PIX_COLOR(TEXT_COLOR), 0, str);
            value *= 2;
            if (value > 2048) break;
        }
        if (value > 2048) break;
    }

    pw_text(pw, 10, WINDOW_HEIGHT - 10, PIX_SRC | PIX_COLOR(TEXT_COLOR), 0, "Demo Mode - Press any key to exit");
}