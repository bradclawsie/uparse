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

// a url is a { scheme, host_port, path, query, fragment }

// scheme is a char *

// -----------------
// path
typedef struct path_t {
    char *path_str;   // (string rep)
    char **path_elts; // (each path elt)
    size_t count; 
} path_t;

// -----------------
// query (key/val pairs)
typedef struct query_key_val_t {
    char *key;
    char *val;
} query_key_val_t;

typedef struct query_arg_list_t {
    query_key_val_t **query_key_vals;
    size_t count;
} query_arg_list_t;

// -----------------
// fragment is a char *

// -----------------
// url
typedef struct url_t {
    char*             scheme;
    char*             host;
    unsigned int      port;
    path_t*           path_elt_list;
    query_arg_list_t* query_arg_list;
    char*             fragment;
    char*             original; // the input string url
} url_t;

// escape a string
char *url_escape(char const *const s);

// -----------------
// parse, init and free urls
url_t *parse_url(char const *const url_string,unsigned int *url_err_out);
void init_url_t(url_t *url);
void free_url_t(url_t *url);

#endif
