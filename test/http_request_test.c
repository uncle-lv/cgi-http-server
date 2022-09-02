#include "http_request.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void usage(const char* name) {
  fprintf(stderr,
          "Usage: %s {filename}\n", name);
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    enum http_parser_type file_type;

    if (argc != 2) {
        usage(argv[0]);
    }

    char *filename = argv[1];

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("fopen");
        goto fail;
    }

    fseek(file, 0, SEEK_END);
    long file_length = ftell(file);
    if (file_length == -1) {
        perror("ftell");
        goto fail;
    }
    fseek(file, 0, SEEK_SET);

    char *data = malloc(file_length);
    if (fread(data, 1, file_length, file) != (size_t)file_length) {
        fprintf(stderr, "couldn't read entire file\n");
        free(data);
        goto fail;
    }

    http_request *request = NULL;
    if (NULL == (request = request_new())) {
        perror("request_new faild");
        goto fail;
    }
    parser_settings_init();
    if (parse_request(request, data) != 0) {
        perror("parse faild");
    }

    size_t iter = 0;
    void *item;
    while (hashmap_iter(request->headers, &iter, &item)) {
        const header *header = item;
        printf("%s: %s\n", header->field, header->value);
    }

    printf("method: %s\n", request->method);
    printf("url: %s\n", request->url);
    printf("body: %s\n", request->body);
    puts("*** get header value ***");
    printf("header: %s\n", get_header_value(request, "Host"));
    request_free(request);
    return 0;
fail:
    fclose(file);
    return -1;
}