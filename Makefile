CC=gcc

all: game

game: game.o console.o ncurses_view.o
	$(CC) -o game game.o console.o ncurses_view.o -lncurses

game.o: game.c game.h
	$(CC) game.c -c -o game.o

console.o: console.c console.h
	$(CC) console.c -c -o console.o

ncurses_view.o: ncurses_view.c ncurses_view.h
	$(CC) ncurses_view.c -lncurses -c -o ncurses_view.o

clean:
	rm -f game game.o console.o ncurses_view.o 
