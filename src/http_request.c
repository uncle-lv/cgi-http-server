#include "http_request.h"
#include <string.h>

static int request_cmp(const void *a, const void *b, void *udata);
static int request_iter(const void *item, void *udata);
static uint64_t request_hash(const void *item, uint64_t seed0, uint64_t seed1);
static int on_message_begin(http_parser* _);
static int on_headers_complete(http_parser* _);
static int on_message_complete(http_parser* _);
static int on_url(http_parser* _, const char* at, size_t length);
static int on_header_field(http_parser* _, const char* at, size_t length);
static int on_header_value(http_parser* _, const char* at, size_t length);
static int on_body(http_parser* _, const char* at, size_t length);

static http_parser_settings parser_settings = {
    on_message_begin,
    on_url,
    NULL,
    on_header_field,
    on_header_value,
    on_headers_complete,
    on_body,
    on_message_complete
};

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

    http_parser_init(&request->parser, HTTP_REQUEST);

    return request;
}