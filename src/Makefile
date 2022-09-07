
all: server

LIBS = -lev http_parser.c hashmap.c http_request.c log.c -DLOG_USE_COLOR

server: server.c
	gcc -Os -Wall -Wextra -o server server.c $(LIBS)

clean:
	rm server