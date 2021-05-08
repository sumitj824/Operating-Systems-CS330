CC = gcc
CFLAGS = -g

umpire: umpire.c player.c
		$(CC) $(CFLAGS) -o umpire umpire.c
		$(CC) $(CFLAGS) -o player1 player.c
		$(CC) $(CFLAGS) -o player2 player.c

test: 	umpire.c tests/test1/player1.c tests/test1/player2.c tests/test2/player1.c tests/test2/player2.c tests/test3/player1 tests/test3/player2 tests/test4/player1 tests/test4/player2 
		$(CC) $(CFLAGS) -o ./tests/test1/player1 ./tests/test1/player1.c
		$(CC) $(CFLAGS) -o ./tests/test1/player2 ./tests/test1/player2.c
		$(CC) $(CFLAGS) -o ./tests/test2/player1 ./tests/test2/player1.c
		$(CC) $(CFLAGS) -o ./tests/test2/player2 ./tests/test2/player2.c
		$(CC) $(CFLAGS) -o ./tests/test3/player1 ./tests/test3/player1.c
		$(CC) $(CFLAGS) -o ./tests/test3/player2 ./tests/test3/player2.c
		$(CC) $(CFLAGS) -o ./tests/test4/player1 ./tests/test4/player1.c
		$(CC) $(CFLAGS) -o ./tests/test4/player2 ./tests/test4/player2.c
clean:
		rm player1 player2 umpire ./tests/test1/player1 ./tests/test1/player2 ./tests/test2/player1 ./tests/test2/player2 \
		./tests/test3/player1 ./tests/test3/player2 ./tests/test4/player1 ./tests/test4/player2 