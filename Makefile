all:
	cc -O 2048sunview.c -o 2048sunview -lsuntool -lsunwindow -lpixrect
	cc -O flap.c -o flap -lsuntool -lsunwindow -lpixrect
	cc -O tetris.c -o tetris -lsuntool -lsunwindow -lpixrect
	cc -O donkey.c -o donkey -lsuntool -lsunwindow -lpixrect
	cc -O bubble.c -o bubble -lsuntool -lsunwindow -lpixrect
	cc -O snake.c -o snake -lsuntool -lsunwindow -lpixrect
	cc -O sokoban.c -o sokoban -lsuntool -lsunwindow -lpixrect
	cc -O mines.c -o mines -lsuntool -lsunwindow -lpixrect

