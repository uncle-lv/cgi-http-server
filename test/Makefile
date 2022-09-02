all: http_request_test

SRC = ../src/
LIBS = $(SRC)http_parser.c $(SRC)hashmap.c $(SRC)http_request.c

http_request_test: http_request_test.c
	gcc -Os -Wall -Wextra -o http_request_test http_request_test.c -I $(SRC) $(LIBS)

clean:
	rm http_request_test