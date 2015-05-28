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
#define OVERFLOW_ERROR  2

// a url is a { scheme, host, port, path, query, fragment }
typedef struct url_t {
    char         *scheme;
    char         *host;
    unsigned int port;
    char         *path;
    char         *query;
    char         *fragment;
} url_t;

// the "pairs" of a query key/val
typedef struct query_key_val_t {
    char *key;
    char *val;
} query_key_val_t;

// a list of query_key_vals and a count
typedef struct query_arg_list_t {
    query_key_val_t **query_key_vals;
    size_t count;
} query_arg_list_t;

// escape a string
char *url_escape(char const *const s);

// parse, init and free urls
url_t *parse_url(char const *const url_string,unsigned int *url_err_out);
void init_url_t(url_t *url);
void free_url_t(url_t *url);
void print_url(url_t *u);

// expand query lists
void free_query_key_val_t(query_key_val_t *query_key_val);
void free_query_key_val_t_list(query_key_val_t **query_key_vals,size_t len);
void free_arg_list_t(query_arg_list_t *query_arg_list);
query_arg_list_t *get_query_arg_list(char *const query_str, unsigned int *err_out);

#endif
