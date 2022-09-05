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
#define HEADER_BUFFER_SIZE 256
#define LOGBACK 20
#define PORT 5000
#define WWW_PATH "../www"

#define RESPONSE_HEADERS "HTTP/1.1 200 OK\r\n\
Content-type: %s\r\n\
Content-length: %ld\r\n\r\n"

static int run_serve(int port);
static void accept_request(EV_P_ ev_io *watcher, int revents);
static void parser_request(EV_P_ ev_io *watcher, int revents);
static void response(EV_P_ ev_io *watcher, int revents);
static void send_file(http_request *request, const char *path);
static long get_file_length(const FILE *file);
static void response_400(http_request *request);
static void response_404(http_request *request);
static void response_500(http_request *request);
static void response_501(http_request *request);


int main(int argc, char *argv[]) {
    int port;

    if (argc == 2) {
        port = atoi(argv[1]);
    } else {
        port = PORT;
    }

    int server_fd = run_serve(port);

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

    if (0 != parse_request(request, data)) {
        request->is_bad_request = true;
    }

    ev_io_stop(EV_A_ watcher);
    ev_io_init(&request->write_watcher, response, watcher->fd, EV_WRITE);
    ev_io_start(EV_A_ &request->write_watcher);
}

static void response(EV_P_ ev_io *watcher, int revents) {
    char path[256];
    http_request *request = REQUEST_FROM_WRITE_WATCHER(watcher);

    if (request->is_bad_request) {
        response_400(request);
        goto finish;
    } else {
        if (strcasecmp(request->method, "GET") && strcasecmp(request->method, "POST")) {
            response_501(request);
            goto finish;
        }

        strcpy(path, WWW_PATH);
        strcat(path, request->url);

        if (0 == strcasecmp(request->method, "GET")) {
            if (0 == strcasecmp(path, "/")) {
                strcat(path, "index.html");
            }

            struct stat st;
            if (-1 == stat(path, &st)) {
                response_404(request);
                goto finish;
            } else {
                if ((st.st_mode & S_IFMT) == S_IFDIR) {
                    strcat(path, "/index.html");
                }
            }

            send_file(request, path);
            goto finish;
        } else {
            
        }
    }

finish:
    ev_io_stop(EV_A_ watcher);
    close(request->client_fd);
    request_free(request);
}

static void send_file(http_request *request, const char *path) {
    FILE *file = NULL;
    char buf[BUFFER_SIZE];

    file = fopen(path, "r");
    if (NULL == file) {
        response_404(request);
    } else {
        sprintf(buf, RESPONSE_HEADERS, "text/html", get_file_length(file));
        send(request->client_fd, buf, strlen(buf), 0);
        fgets(buf, BUFFER_SIZE, file);
        send(request->client_fd, buf, strlen(buf), 0);
        while (!feof(file)) {
            fgets(buf, BUFFER_SIZE, file);
            send(request->client_fd, buf, strlen(buf), 0);
        }
    }
}

static long get_file_length(const FILE *file) {
    long cur = ftell(file);
    fseek(file, 0, SEEK_END);
    long file_length = ftell(file);
    fseek(file, cur, SEEK_SET);
    return file_length;
}

static void response_400(http_request *request) {
    char path[128];
    memset(path, 0, sizeof(path));
    strcat(path, WWW_PATH);
    strcat(path, "/400.html");
    send_file(request, path);
}

static void response_404(http_request *request) {
    char path[128];
    memset(path, 0, sizeof(path));
    strcat(path, WWW_PATH);
    strcat(path, "/404.html");
    send_file(request, path);
}

static void response_500(http_request *request) {
    char path[128];
    memset(path, 0, sizeof(path));
    strcat(path, WWW_PATH);
    strcat(path, "/500.html");
    send_file(request, path);
}

static void response_501(http_request *request) {
    char path[128];
    memset(path, 0, sizeof(path));
    strcat(path, WWW_PATH);
    strcat(path, "/501.html");
    send_file(request, path);
}

/**
 * @brief 启动HTTP服务器
 * 
 * @param port 服务器端口号
 * @return int 服务器文件描述符
 */
static int run_serve(int port) {
    struct sockaddr_in server_addr;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    listen(server_fd, LOGBACK);
    parser_settings_init();
    printf("HTTP server is running at %d\n", port);
    return server_fd;
}