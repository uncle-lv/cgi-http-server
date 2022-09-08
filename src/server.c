#include "http_request.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <ev.h>

#define STDIN   0
#define STDOUT  1
#define STDERR  2

#define BUFFER_SIZE 2048
#define HEADER_BUFFER_SIZE 256
#define LOGBACK 20
#define PORT 5000
#define WWW_PATH "../www"
#define CGI_PATH "/cgi-bin"

#define RESPONSE_HEADERS "HTTP/1.0 %d OK\r\n\
Content-type: %s\r\n\
Content-length: %ld\r\n\r\n"

static int run_serve(int port);
static void accept_request(EV_P_ ev_io *watcher, int revents);
static void parse_request_data(EV_P_ ev_io *watcher, int revents);
static void response(EV_P_ ev_io *watcher, int revents);
static void send_file(http_request *request, const char *path, int status);
static long get_file_length(FILE *file);
static int execute_cgi(http_request *request, const char *path);
static void response_400(http_request *request);
static void response_404(http_request *request);
static void response_500(http_request *request);
static void response_501(http_request *request);
static int check_file_exists(const char *path, int mode);


int main(int argc, char *argv[]) {
    int port;

    if (argc == 2) {
        port = atoi(argv[1]);
    } else {
        port = PORT;
        log_warn("The server port is not assigned. Using port %d by default.", port);
    }

    int server_fd = run_serve(port);

    struct ev_loop *loop = EV_DEFAULT;
    ev_io server_watcher;
    ev_io_init(&server_watcher, accept_request, server_fd, EV_READ);
    ev_io_start(loop, &server_watcher);

    ev_run(loop, 0);

    return 0;
}

static void accept_request(EV_P_ ev_io *watcher, int _) {
    (void)_;
    char addr_str[INET_ADDRSTRLEN];
    struct sockaddr_in client_addr;
    socklen_t clientaddr_len = sizeof(client_addr);
    bzero(addr_str, sizeof(addr_str));
    int client_fd = accept(watcher->fd, (struct sockaddr *)&client_addr, &clientaddr_len);
    if (-1 == client_fd) {
        return;
    }

    http_request *request = request_new();
    request->client_fd = client_fd;
    request->client_addr = client_addr;
    ev_io_init(&request->read_watcher, parse_request_data, client_fd, EV_READ);
    ev_io_start(EV_A_ &request->read_watcher);
}

static void parse_request_data(EV_P_ ev_io *watcher, int _) {
    (void)_;
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

    if (0 != parse_request(request, data, data_len)) {
        request->is_bad_request = true;
    }

    ev_io_stop(EV_A_ watcher);
    ev_io_init(&request->write_watcher, response, watcher->fd, EV_WRITE);
    ev_io_start(EV_A_ &request->write_watcher);
}

static void response(EV_P_ ev_io *watcher, int _) {
    (void)_;
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

        if (!request->is_cgi) {
            strcat(path, request->path);
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

            if (check_file_exists(path, R_OK)) {
                response_404(request);
                goto finish;
            }

            send_file(request, path, HTTP_STATUS_OK);
            goto finish;
        } else {
            strcat(path, CGI_PATH);
            strcat(path, request->path);

            if (check_file_exists(path, X_OK)) {
                response_404(request);
            }

            execute_cgi(request, path);
        }
    }

finish:
    ev_io_stop(EV_A_ watcher);
    close(request->client_fd);
    request_free(request);
}

static void send_file(http_request *request, const char *path, int status) {
    FILE *file = NULL;
    char buf[BUFFER_SIZE];

    file = fopen(path, "r");
    sprintf(buf, RESPONSE_HEADERS, status, "text/html", get_file_length(file));
    send(request->client_fd, buf, strlen(buf), 0);
    fgets(buf, BUFFER_SIZE, file);
    send(request->client_fd, buf, strlen(buf), 0);
    while (!feof(file)) {
        fgets(buf, BUFFER_SIZE, file);
        send(request->client_fd, buf, strlen(buf), 0);
    }

    log_info("%s:%u - \"%s %s HTTP/%u.%u\" %d %s",
             inet_ntoa(request->client_addr.sin_addr), request->client_addr.sin_port, request->method, request->path, request->http_major, request->http_minor, status, http_status_str(status));
}

static long get_file_length(FILE *file) {
    long cur = ftell(file);
    fseek(file, 0, SEEK_END);
    long file_length = ftell(file);
    fseek(file, cur, SEEK_SET);
    return file_length;
}

static int execute_cgi(http_request *request, const char *path) {
    char buf[BUFFER_SIZE];
    int cgi_output[2];
    int cgi_input[2];
    char content_length[16];
    pid_t pid;
    int status;

    memset(buf, 0, BUFFER_SIZE);
    memset(content_length, 0, sizeof(content_length));

    if (pipe(cgi_output) < 0) {
        return -1;
    }
    
    if (pipe(cgi_input) < 0) {
        return -1;
    }

    if ((pid = fork()) < 0) {
        return -1;
    }

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(request->client_fd, buf, strlen(buf), 0);
    if (0 == pid) {
        dup2(cgi_output[1], STDOUT);
        dup2(cgi_input[0], STDIN);
        close(cgi_output[0]);
        close(cgi_input[1]);

        setenv("REQUEST_METHOD", request->method, 1);
        if (0 == strcasecmp(request->method, "GET")) {
            setenv("QUERY_STRING", request->query_string, 1);
        } else {
            sprintf(content_length, "%lu", strlen(request->body));
            setenv("CONTENT_LENGTH", content_length, 1);
        }

        if (-1 == execl(path, NULL)) {
            perror("execl: ");
            response_500(request);
        }
        exit(0);
    } else {
        close(cgi_output[1]);
        close(cgi_input[0]);
        if (0 == strcasecmp(request->method, "POST")) {
            write(cgi_input[1], request->body, strlen(request->body));
        }

        while (read(cgi_output[0], buf, BUFFER_SIZE) > 0) {
            send(request->client_fd, buf, BUFFER_SIZE, 0);
        }

        close(cgi_output[0]);
        close(cgi_input[0]);
        waitpid(pid, &status, 0);
    }

    log_info("%s:%u - \"%s %s HTTP/%u.%u\" %d %s",
             inet_ntoa(request->client_addr.sin_addr), request->client_addr.sin_port, request->method, request->path, request->http_major, request->http_minor, HTTP_STATUS_OK, http_status_str(HTTP_STATUS_OK));

    return 0;
}

static void response_400(http_request *request) {
    char path[128];
    memset(path, 0, sizeof(path));
    strcat(path, WWW_PATH);
    strcat(path, "/400.html");
    send_file(request, path, HTTP_STATUS_BAD_REQUEST);
}

static void response_404(http_request *request) {
    char path[128];
    memset(path, 0, sizeof(path));
    strcat(path, WWW_PATH);
    strcat(path, "/404.html");
    send_file(request, path, HTTP_STATUS_NOT_FOUND);
}

static void response_500(http_request *request) {
    char path[128];
    memset(path, 0, sizeof(path));
    strcat(path, WWW_PATH);
    strcat(path, "/500.html");
    send_file(request, path, HTTP_STATUS_INTERNAL_SERVER_ERROR);
}

static void response_501(http_request *request) {
    char path[128];
    memset(path, 0, sizeof(path));
    strcat(path, WWW_PATH);
    strcat(path, "/501.html");
    send_file(request, path, HTTP_STATUS_NOT_IMPLEMENTED);
}

/**
 * @brief 启动HTTP服务器
 * 
 * @param port 服务器端口号
 * @return int 服务器文件描述符
 */
static int run_serve(int port) {
    struct sockaddr_in server_addr;
    int server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    listen(server_fd, LOGBACK);
    parser_settings_init();
    log_info("HTTP server is running at %d", port);
    return server_fd;
}

static int check_file_exists(const char *path, int mode) {
    if (!access(path, mode)) {
        return 0;
    }
    return -1;
}