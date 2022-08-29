#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ev.h>

#define MAXLINE 80
#define LOGBACK 5

static int run_serve(int port);
static void accept_request(EV_P_ ev_io *watcher, int revents);
static void parser_data(EV_P_ ev_io *watcher, int revents);


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fputs("usage: ./server {port}\n", stderr);
        exit(1);
    }

    int port = atoi(argv[1]);
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
    ev_io *client_watcher = malloc(sizeof(ev_io));
    memset(client_watcher, 0, sizeof(ev_io));
    ev_io_init(client_watcher, parser_data, client_fd, EV_READ);
    ev_io_start(EV_A_ client_watcher);
}

static void parser_data(EV_P_ ev_io *watcher, int revents) {
    int client_fd = watcher->fd;
    char buf[MAXLINE];
    bzero(buf, sizeof(buf));
    int n = recv(client_fd, buf, MAXLINE, 0);
    if (n < 0) {
        ev_io_stop(EV_A_ watcher);
        free(watcher);
        close(client_fd);
    } else if (0 == n) {
        ev_io_stop(EV_A_ watcher);
        free(watcher);
        close(client_fd);
    } else {
        printf("received message: %s\n", buf);
        send(client_fd, buf, strlen(buf) < MAXLINE ? strlen(buf) : MAXLINE, 0);
    }
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