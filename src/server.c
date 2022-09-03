#include "http_request.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ev.h>

#define BUFFER_SIZE 2048
#define LOGBACK 5
#define PORT 5000

#define BAD_REQUEST "HTTP/1.1 400 Bad Request\r\n\r\n"
#define HELLO "HTTP/1.1 200 OK\r\n\r\nHello, World!"

static int run_serve(int port);
static void accept_request(EV_P_ ev_io *watcher, int revents);
static void parser_request(EV_P_ ev_io *watcher, int revents);
static void response(EV_P_ ev_io *watcher, int revents);


int main(int argc, char *argv[]) {
    int port;

    if (argc == 2) {
        port = atoi(argv[1]);
    } else {
        port = PORT;
    }

    int server_fd = run_serve(port);
    printf("HTTP server is running at %d\n", port);

    struct ev_loop *loop = EV_DEFAULT;
    ev_io server_watcher;
    ev_io_init(&server_watcher, accept_request, server_fd, EV_READ);
    ev_io_start(loop, &server_watcher);

    ev_run(loop, 0);

    return 0;
}

static void accept_request(EV_P_ ev_io *watcher, int revents) {
    char addr_str[INET_ADDRSTRLEN];
    struct sockaddr_in client_addr;
    socklen_t clientaddr_len = sizeof(client_addr);
    bzero(addr_str, sizeof(addr_str));
    int client_fd = accept(watcher->fd, (struct sockaddr *)&client_addr, &clientaddr_len);
    if (-1 == client_fd) {
        return;
    }

    printf("--- accept connection from %s ---\n", inet_ntop(AF_INET, &client_addr.sin_addr, addr_str, sizeof(addr_str)));
    http_request *request = request_new();
    request->client_fd = client_fd;
    request->client_addr = client_addr;
    ev_io_init(&request->read_watcher, parser_request, client_fd, EV_READ);
    ev_io_start(EV_A_ &request->read_watcher);
}

static void parser_request(EV_P_ ev_io *watcher, int revents) {
    http_request *request = REQUEST_FROM_READ_WATCHER(watcher);
    char *data = NULL;
    char buf[BUFFER_SIZE];
    bzero(buf, sizeof(buf));
    int nread = 0;
    int data_len = 0;
    do {
        nread = read(watcher->fd, &buf, BUFFER_SIZE);
        data_len += nread;
        if (data_len < BUFFER_SIZE) {
            buf[nread] = '\0';
        }

        if (NULL == data) {
            data = malloc(nread+1);
            memcpy(data, buf, nread);
        } else {
            data = realloc(data, data_len+1);
            memcpy(data + data_len - nread, buf, nread);
        }
    }  while (nread == BUFFER_SIZE);

    if (1 == parse_request(request, data)) {
        request->is_bad_request = true;
    }

    ev_io_stop(EV_A_ watcher);
    ev_io_init(&request->write_watcher, response, watcher->fd, EV_WRITE);
    ev_io_start(EV_A_ &request->write_watcher);
}

static void response(EV_P_ ev_io *watcher, int revents) {
    http_request *request = REQUEST_FROM_WRITE_WATCHER(watcher);
    if (request->is_bad_request) {
        send(watcher->fd, BAD_REQUEST, strlen(BAD_REQUEST), 0);
        return;
    } else {
        send(watcher->fd, HELLO, strlen(HELLO), 0);
    }
    
    ev_io_stop(EV_A_ watcher);
    close(request->client_fd);
    request_free(request);
}

static int run_serve(int port) {
    struct sockaddr_in server_addr;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    listen(server_fd, LOGBACK);
    return server_fd;
}