
all: server

LIBS = -lev

server: server.c
	gcc -Os -Wall -Wextra -o server server.c $(LIBS)

clean:
	rm server