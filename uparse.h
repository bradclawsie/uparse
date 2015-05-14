#ifndef UPARSE_H
#define UPARSE_H

#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define NO_UPARSE_ERROR 0
#define UPARSE_ERROR    1

// a url is a { scheme, host, port, path, query, fragment }
typedef struct url_t {
    char         *scheme;
    char         *host;
    unsigned int port;
    char         *path;
    char         *query;
    char         *fragment;
} url_t;

// escape a string
char *url_escape(char const *const s);

// parse, init and free urls
url_t *parse_url(char const *const url_string,unsigned int *url_err_out);
void init_url_t(url_t *url);
void free_url_t(url_t *url);
void print_url(url_t *u);

#endif
