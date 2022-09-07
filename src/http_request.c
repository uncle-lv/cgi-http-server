#include "http_request.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define REQUEST ((http_request *)parser->data)

static int request_cmp(const void *a, const void *b, void *udata);
static bool request_iter(const void *item, void *udata);
static uint64_t request_hash(const void *item, uint64_t seed0, uint64_t seed1);
static int on_message_begin(http_parser* parser);
static int on_headers_complete(http_parser* parser);
static int on_message_complete(http_parser* parser);
static int on_chunk_header(http_parser* parser);
static int on_chunk_complete(http_parser* parser);
static int on_url(http_parser* parser, const char* at, size_t length);
static int on_status(http_parser* parser, const char* at, size_t length);
static int on_header_field(http_parser* parser, const char* at, size_t length);
static int on_header_value(http_parser* parser, const char* at, size_t length);
static int on_body(http_parser* parser, const char* at, size_t length);
static void set_or_append_header(http_request *request, const char *field, const char *value);

static http_parser_settings parser_settings;

/**
 * @brief 初始化parser_settings，在启动服务器时调用
 */
void parser_settings_init() {
    memset(&parser_settings, 0, sizeof(parser_settings));
    parser_settings.on_message_begin = on_message_begin;
    parser_settings.on_url = on_url;
    parser_settings.on_status = on_status;
    parser_settings.on_header_field = on_header_field;
    parser_settings.on_header_value = on_header_value;
    parser_settings.on_headers_complete = on_headers_complete;
    parser_settings.on_body = on_body;
    parser_settings.on_message_complete = on_message_complete;
}

http_request *request_new() {
    http_request *request = malloc(sizeof(http_request));
    if (NULL == request) {
        return NULL;
    }

    memset(request, 0, sizeof(http_request));
    request->headers = hashmap_new(
        sizeof(header),
        0, 0, 0,
        request_hash,
        request_cmp,
        NULL, NULL
    );

    http_parser_init((http_parser *)&request->parser, HTTP_REQUEST);
    request->parser.data = request;

    return request;
}

/**
 * @brief 解析请求
 * 
 * @param request request结构体
 * @param data http请求数据
 * @return int 解析失败返回-1，成功返回0
 */
int parse_request(http_request *request, char *data, size_t len) {
    char suffix[16];
    bzero(suffix, sizeof(suffix));
    bzero(request->method, sizeof(request->method));
    bzero(request->query_string, sizeof(request->query_string));
    bzero(request->path, sizeof(request->path));
    size_t nparsed = http_parser_execute(&request->parser, &parser_settings, data, len);
    if (nparsed != len) {
        return -1;
    }

    request->http_major = request->parser.http_major;
    request->http_minor = request->parser.http_minor;

    const char *method = http_method_str(request->parser.method);
    sprintf(request->method, "%s", method);
    sscanf(request->url, "%*[^.].%3s", suffix);
    sscanf(request->url, "%*[^?]?%s", request->query_string);
    sscanf(request->url, "%[^?]", request->path);
    if (0 == strcasecmp(suffix, "cgi")) {
        request->is_cgi = 1;
    }
    free(data);
    data = NULL;
    return 0;
}

void request_free(http_request *request) {
    if (request->headers != NULL) {
        hashmap_free(request->headers);
        request->headers = NULL;
    }

    if (request->body != NULL) {
        free(request->body);
        request->body = NULL;
    }

    if (request != NULL) {
        free(request);
        request = NULL;
    }
}

const char *get_header_value(http_request *request, const char *field) {
    header *h = hashmap_get(request->headers, &(header){ .field=field });
    return h->value;
}

static int request_cmp(const void *a, const void *b, void *_) {
    (void)_;
    const header *ua = a;
    const header *ub = b;
    return strcmp(ua->field, ub->field);
}

static bool request_iter(const void *item, void *_) {
    (void)_;
    const header *header = item;
    printf("%s: %s\n", header->field, header->value);
    return true;
}

static uint64_t request_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const header *header = item;
    return hashmap_sip(header->field, strlen(header->field), seed0, seed1);
}

static int on_message_begin(http_parser* parser) {
    (REQUEST)->last_call_was_on_header_field = false;
    bzero((REQUEST)->header_field, sizeof((REQUEST)->header_field));
    return 0;
}

static int on_headers_complete(http_parser* parser) {
    (REQUEST)->last_call_was_on_header_field = false;    
    bzero((REQUEST)->header_field, sizeof((REQUEST)->header_field));
    return 0;
}

static int on_message_complete(http_parser* _) {
    (void)_;
    return 0;
}

static int on_chunk_header(http_parser* _) {
    (void)_;
    return 0;
}

static int on_chunk_complete(http_parser* _) {
    (void)_;
    return 0;
}

static int on_url(http_parser* parser, const char* at, size_t length) {
    bzero((REQUEST)->url, sizeof((REQUEST)->url));
    sprintf((REQUEST)->url, "%.*s", (int)length, at);
    return 0;
}

static int on_status(http_parser* parser, const char* at, size_t length) {
    (void)parser;
    (void)at;
    (void)length;
    return 0;
}

static int on_header_field(http_parser* parser, const char* at, size_t length) {
    (REQUEST)->last_call_was_on_header_field = true;
    bzero((REQUEST)->header_field, sizeof((REQUEST)->header_field));
    sprintf((REQUEST)->header_field, "%.*s", (int)length, at);
    return 0;
}

static int on_header_value(http_parser* parser, const char* at, size_t length) {
    char *value = malloc(sizeof(char)*(length+1));
    bzero(value, sizeof(char)*(length+1));
    if ((REQUEST)->last_call_was_on_header_field && (REQUEST)->header_field != NULL) {
        char *header = malloc(sizeof(char)*(strlen((REQUEST)->header_field)+1));
        bzero(header, sizeof(char)*(strlen((REQUEST)->header_field)+1));
        sprintf(header, "%s", (REQUEST)->header_field);
        sprintf(value, "%.*s", (int)length, at);
        set_or_append_header((REQUEST), header, value);
    }

    (REQUEST)->last_call_was_on_header_field = false;
    
    return 0;
}

static int on_body(http_parser* parser, const char* at, size_t length) {
    (REQUEST)->body = malloc(sizeof(char)*(length+1));
    bzero((REQUEST)->body, sizeof(length+1));
    sprintf((REQUEST)->body, "%.*s", (int)length, at);
    return 0;
}

static void set_or_append_header(http_request *request, const char *field, const char *value) {
    hashmap_set(request->headers, &(header){ .field=field, .value=value });
}