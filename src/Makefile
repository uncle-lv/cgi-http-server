
all: server

LIBS = -lev http_parser.c hashmap.c http_request.c

server: server.c
	gcc -Os -Wall -Wextra -o server server.c $(LIBS)

clean:
	rm server