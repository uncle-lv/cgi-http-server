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

    memset(request, 0, sizeof(request));
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

int parse_request(http_request *request, const char *data) {
    size_t data_len = strlen(data);
    size_t nparsed = http_parser_execute(&request->parser, &parser_settings, data, data_len);
    if (nparsed != data_len) {
        return 1;
    }

    char *method = http_method_str(request->parser.method);
    request->method = malloc(sizeof(char)*strlen(method)+1);
    sprintf(request->method, "%s", method);
    free(data);
    data = NULL;
    return 0;
}

void request_free(http_request *request) {
    if (request->headers != NULL) {
        hashmap_free(request->headers);
        request->headers = NULL;
    }

    if (request->method != NULL) {
        free(request->method);
        request->method = NULL;
    }

    if (request->url != NULL) {
        free(request->url);
        request->url = NULL;
    }

    if (request->body != NULL) {
        free(request->body);
        request->body = NULL;
    }
    
    if (request->header_field != NULL) {
        free(request->header_field);
        request->header_field = NULL;
    }

    if (request != NULL) {
        free(request);
        request = NULL;
    }
}

char *get_header_value(http_request *request, const char *field) {
    header *h = hashmap_get(request->headers, &(header){ .field=field });
    return h->value;
}

static int request_cmp(const void *a, const void *b, void *udata) {
    const header *ua = a;
    const header *ub = b;
    return strcmp(ua->field, ub->field);
}

static bool request_iter(const void *item, void *udata) {
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
    (REQUEST)->header_field = NULL;
    return 0;
}

static int on_headers_complete(http_parser* parser) {
    (REQUEST)->last_call_was_on_header_field = false;
    (REQUEST)->header_field = NULL;
    return 0;
}

static int on_message_complete(http_parser* parser) {
    return 0;
}

static int on_chunk_header(http_parser* parser) {
    return 0;
}

static int on_chunk_complete(http_parser* parser) {
    return 0;
}

static int on_url(http_parser* parser, const char* at, size_t length) {
    (REQUEST)->url = malloc(sizeof(char)*(length+1));
    memset((REQUEST)->url, 0, sizeof((REQUEST)->url));
    sprintf((REQUEST)->url, "%.*s", (int)length, at);
    return 0;
}

static int on_status(http_parser* parser, const char* at, size_t length) {
    return 0;
}

static int on_header_field(http_parser* parser, const char* at, size_t length) {
    (REQUEST)->last_call_was_on_header_field = true;

    (REQUEST)->header_field = malloc(sizeof(char)*(length+1));
    memset((REQUEST)->header_field, 0, sizeof((REQUEST)->header_field));
    sprintf((REQUEST)->header_field, "%.*s", (int)length, at);
    return 0;
}

static int on_header_value(http_parser* parser, const char* at, size_t length) {
    char *value = malloc(sizeof(char)*(length+1));
    memset(value, 0, sizeof(value));
    if ((REQUEST)->last_call_was_on_header_field && (REQUEST)->header_field != NULL) {
        sprintf(value, "%.*s", (int)length, at);
        set_or_append_header((REQUEST), (REQUEST)->header_field, value);
    }

    (REQUEST)->last_call_was_on_header_field = false;
    (REQUEST)->header_field = NULL;
    
    return 0;
}

static int on_body(http_parser* parser, const char* at, size_t length) {
    (REQUEST)->body = malloc(sizeof(char)*(length+1));
    memset((REQUEST)->body, 0, sizeof((REQUEST)->body));
    sprintf((REQUEST)->body, "%.*s", (int)length, at);
    return 0;
}

static void set_or_append_header(http_request *request, const char *field, const char *value) {
    hashmap_set(request->headers, &(header){ .field=field, .value=value });
}