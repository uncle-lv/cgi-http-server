#include "hashmap.h"
#include "http_parser.h"
#include <ev.h>
#include <netinet/in.h>

typedef struct {
    const char *field;
    const char *value;
} header;

typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;

    ev_io read_watcher;
    ev_io write_watcher;

    http_parser parser;
    unsigned int http_major;
    unsigned int http_minor;
    char method[16];
    char url[256]; // url存储的是整个url
    char path[64]; // path只存储url中的文件路径
    struct hashmap *headers;
    char *body;
    char query_string[128];

    int is_cgi;
    int is_bad_request;

    /**
     * http parser在解析http头部时，会逐次调用on_header_field、on_header_value回调。
     * 有多少个头部字段，就调用on_header_field、on_header_value多少次。
     * 以下两个字段分别用于记录on_header_value的上次回调是否on_header_field
     * 和上一次on_header_field所获取到的header field
     */
    int last_call_was_on_header_field;
    char header_field[64];
} http_request;

// 这两个宏用于计算地址偏移获取request
#define REQUEST_FROM_READ_WATCHER(watcher) \
  (http_request*)((size_t)watcher - (size_t)(&(((http_request*)NULL)->read_watcher)));

#define REQUEST_FROM_WRITE_WATCHER(watcher) \
  (http_request*)((size_t)watcher - (size_t)(&(((http_request*)NULL)->write_watcher)));

void parser_settings_init();
http_request *request_new();
int parse_request(http_request *request, char *data, size_t len);
void request_free(http_request *request);
const char *get_header_value(http_request *request, const char *field);
