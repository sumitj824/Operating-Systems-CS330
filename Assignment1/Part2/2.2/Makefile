CC = gcc
CFLAGS = -g -o

umpire2: umpire2.c player.c gameUtils.c
		$(CC) $(CFLAGS) umpire2 umpire2.c gameUtils.c
		$(CC) $(CFLAGS) player1 player.c
		$(CC) $(CFLAGS) player2 player.c

test: 	tests/test1/player1.c tests/test1/player2.c tests/test2/player1.c tests/test2/player2.c
		$(CC) $(CFLAGS) ./tests/test1/player1 ./tests/test1/player1.c
		$(CC) $(CFLAGS) ./tests/test1/player2 ./tests/test1/player2.c
		$(CC) $(CFLAGS) ./tests/test2/player1 ./tests/test2/player1.c
		$(CC) $(CFLAGS) ./tests/test2/player2 ./tests/test2/player2.c
		$(CC) $(CFLAGS) ./tests/test3/player1 ./tests/test3/player1.c
		$(CC) $(CFLAGS) ./tests/test3/player2 ./tests/test3/player2.c
		$(CC) $(CFLAGS) ./tests/test3/player3 ./tests/test3/player3.c
		$(CC) $(CFLAGS) ./tests/test4/player1 ./tests/test4/player1.c
		$(CC) $(CFLAGS) ./tests/test4/player2 ./tests/test4/player2.c
		$(CC) $(CFLAGS) ./tests/test4/player3 ./tests/test4/player3.c

clean:
		rm umpire2 player1 player2 ./tests/test1/player1 ./tests/test1/player2 ./tests/test2/player1 ./tests/test2/player2 \
		./tests/test3/player1 ./tests/test3/player2 ./tests/test3/player3 \
		./tests/test4/player1 ./tests/test4/player2 ./tests/test4/player3
