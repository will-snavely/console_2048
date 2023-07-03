CC=gcc

all: game

game: game.o console_model.o ncurses_view.o
	$(CC) -o game game.o console_model.o ncurses_view.o -lncurses

game.o: game.c game.h
	$(CC) game.c -c -o game.o

console_model.o: console_model.c console_model.h
	$(CC) console_model.c -c -o console_model.o

ncurses_view.o: ncurses_view.c ncurses_view.h
	$(CC) ncurses_view.c -lncurses -c -o ncurses_view.o

clean:
	rm -f game game.o console_model.o ncurses_view.o 
