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
    int last_call_was_on_header_field;
    char *header_field;
} http_request;

void parser_settings_init();
http_request *request_new();
int parse_request(http_request *request, const char *data);
void request_free(http_request *request);
char *get_header_value(http_request *request, const char *field);
