CC     = gcc
CFLAGS = -Wall -Wextra -O2

all: taquin_vs

taquin_vs: taquin_vs.c board.c list.c board.h list.h item.h
	$(CC) $(CFLAGS) -o taquin_vs taquin_vs.c board.c list.c -lm

list.o: list.c list.h
	$(CC) $(CFLAGS) -c list.c

board.o: board.c board.h
	$(CC) $(CFLAGS) -c board.c -lm

clean:
	rm -f taquin_vs *.o


