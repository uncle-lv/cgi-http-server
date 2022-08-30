#include "hashmap.h"
#include "http_parser.h"

typedef struct {
    char *field;
    char *value;
} header;

typedef struct {
    http_parser parser;
    char *method;
    char *url;
    struct hashmap *headers;
    char *body;
} http_request;

http_request *request_new();
int request_parser(const char *data);
void request_free(http_request *request);
