#include "uparse.h"

static int const NO_PORT                  = 0;
static int const ERROR_PORT               = -1;

static char const SCHEME_DELIM_PREFIX     = ':';
static char const SCHEME_SLASH            = '/';
static char const PATH_DELIM              = '/';
static char const HOST_PORT_DELIM         = ':';
static char const DOMAIN_DELIM            = '.';
static char const QUERY_DELIM             = '?';
static char const QUERY_KEY_VAL_DELIM     = '='; 
static char const FRAGMENT_DELIM          = '#';
static char const QUERY_PAIR_DELIM        = '&'; 

#define ESCAPE_CHARS_COUNT 19

static char const ESCAPE_CHARS[ESCAPE_CHARS_COUNT] =
    {'!','#','$','%','&','\'','(',')','*','+',
     ',','/',':',';','=','?','@',']','['};
static char const *const PERCENT_REPLACE[ESCAPE_CHARS_COUNT] =
    {"%21","%23","%24","%25","%26","%27","%28","%29","%2A","%2B",
     "%2C","%2F","%3A","%3B","%3D","%3F","%40","%5B","%5D"};


// -----------------------------------------

// Url escape a string. Assumes all chars will need to be replaced,
// allocates enough space to do so.

char *url_escape(char const *const s) {

    // Build an array large enough to support every char being escaped (replaced by three chars).
    size_t const c_esc_len = (3 * strlen(s)) + 1;
    char *esc_s = (char *) calloc(c_esc_len,sizeof(char));
    if (NULL == esc_s) {
        fprintf(stderr,"cannot allocate esc_s\n");
        return NULL;
    }

    char const *c = s;

    size_t j = 0;
    bool percent_replaced = false;;

    while (*c) {
        percent_replaced = false;
        for (size_t i = 0; i < ESCAPE_CHARS_COUNT; i++) {
            if (ESCAPE_CHARS[i] == *c) {
                // *c needs to be replaced in esc_s with PERCENT_REPLACE[i].
                char const *t = PERCENT_REPLACE[i];
                for (size_t k = 0; k < 3; k++) {
                    esc_s[j++] = *t++;
                }
                percent_replaced = true;
                break;
            }
        }
        if (!percent_replaced) {
            // *c was not a escape char.
            esc_s[j++] = *c;
        }
        c++;
    }

    esc_s[j] = '\0';
    return esc_s;
}


// -----------------------------------------
// a url is a { scheme, host_port, path, query, fragment }

void init_url_t(url_t *url) {
    url->scheme         = NULL;
    url->host           = NULL;
    url->port           = 0;
    url->path           = NULL;
    url->query          = NULL;
    url->fragment       = NULL;
}

void free_url_t(url_t *url) {
    free(url->scheme);
    url->scheme = NULL;
    free(url->host);
    url->host = NULL;
    free(url->path);
    url->path = NULL;
    free(url->query);
    url->query = NULL;
    free(url->fragment);
    url->fragment = NULL;
    free(url);
}


// -----------------------------------------
// SCHEME PARSING

// Get the protocol scheme, and advance past the expected ://
// Every url has a scheme. If this returns NULL, it is an error

static char *get_protocol_scheme(char **s, unsigned int *err_out) {

    *err_out = UPARSE_ERROR;

    if (NULL == *s) {
        fprintf(stderr,"input s is null\n");
        return NULL;
    }

    // Local copy we can advance.
    char *c = *s;

    // Choose a sensible limit for a scheme.
    size_t const max_scheme_len = 16;
    char scheme[max_scheme_len+1];
    memset(&scheme[0], 0, sizeof(scheme));

    size_t j = 0;

    bool seen_prefix = false;

    // Copy alpha characters preceeding the ':'
    while (*c) {
        if (SCHEME_DELIM_PREFIX == c[0]) {
            seen_prefix = true;
            // Advance past SCHEME_DELIM_PREFIX, c should be pointing at a SCHEME_SLASH now.
            c++;
            break;
        } else if (!isalpha(*c)) {
            fprintf(stderr,"'%c' is invalid\n",*c);
            return NULL;
        } else if (max_scheme_len == j) {
            fprintf(stderr,"scheme exceeds max scheme len %lu\n",max_scheme_len);
            return NULL;
        } else {
            scheme[j] = c[0];
        }
        c++;
        j++;
    }

    if (!seen_prefix) {
        fprintf(stderr,"no scheme delimiter prefix '%c' found\n",SCHEME_DELIM_PREFIX);
        return NULL;
    }

    // No scheme was found.
    if (0 == j) {
        fprintf(stderr,"no scheme was found\n");
        return NULL;
    }

    // Look for the slashes after SCHEME_DELIM_PREFIX.
    bool seen_slash = false;
    while (*c) {
        if (SCHEME_SLASH == c[0]) {
            seen_slash = true;
            c++;
        } else {
            break;
        }
    }

    if (!seen_slash) {
        fprintf(stderr,"no scheme slash '%c' found\n",SCHEME_SLASH);
        return NULL;
    }

    // Advance pointer past all of the scheme chars and delims.
    *s = c;

    scheme[j] = '\0';
    *err_out = NO_UPARSE_ERROR;
    return strdup(scheme);
}


// -----------------------------------------
// HOST PARSING

// Get the host section of the url. Doesn't support ipv6, unicode hosts,
// username annotations etc.
// Every url must have a host. If this returns NULL, it is an error.

char *get_host(char **s, unsigned int *err_out) {

    *err_out = UPARSE_ERROR;

    if (NULL == *s) {
        fprintf(stderr,"input s is null\n");
        return NULL;
    }

    // Local copy we can advance.
    char *c = *s;

    // Choose a sensible limit for a host.
    size_t const max_host_len = 128;
    char host[max_host_len+1];
    memset(&host[0], 0, sizeof(host));

    size_t j = 0;

    while (*c) {

        if ((HOST_PORT_DELIM == c[0]) ||
            (PATH_DELIM      == c[0]) ||
            (QUERY_DELIM     == c[0]) ||
            (FRAGMENT_DELIM  == c[0])) {
            break;
        } else if (!(isalnum(c[0]) || (DOMAIN_DELIM == c[0]))) {
            fprintf(stderr,"host has whitespace '%c'\n",c[0]);
            return NULL;
        } else if (max_host_len == j) {
            fprintf(stderr,"host exceeds max host len %lu\n",max_host_len);
            return NULL;
        } else {
            host[j] = c[0];
        }
        c++;
        j++;
    }

    // We didn't set a host.
    if (0 == j) {
        fprintf(stderr,"no host was found\n");
        return NULL;
    }

    // Advance pointer past the host.
    *s = c;

    host[j] = '\0';
    *err_out = NO_UPARSE_ERROR;
    return strdup(host);
}


// -----------------------------------------
// PORT PARSING
// The port part of the url is optional. If it is not specified, the
// default in url_t is 0, and it should be assumed that the default
// port set in /etc/services should be used for the protocol
//
// The return value is int, whereas the port in url_t is an
// unsigned int, so in the case of an error, we can return
// ERROR_PORT (-1), which cannot be assigned to the port part of url_t.

int get_port(char **s, unsigned int *err_out) {

    *err_out = UPARSE_ERROR;

    if (NULL == *s) {
        fprintf(stderr,"input s is null\n");
        return ERROR_PORT;
    }

    // If the string is empty, that means there was nothing after
    // the host (no port, no path), so we return 0.
    if (strlen(*s) == 0) {
        *err_out = NO_UPARSE_ERROR;
        return NO_PORT;
    }

    // Local copy we can advance.
    char *c = *s;

    // If the first char is not ':', then there is no port designation.
    if (HOST_PORT_DELIM != c[0]) {
        *err_out = NO_UPARSE_ERROR;
        *s = c;
        return NO_PORT;
    }

    // Advance past the ':'
    c++;

    // Choose a sensible limit for a port string.
    size_t const max_port_chars_len = 6;
    char port_chars[max_port_chars_len+1];
    memset(&port_chars[0], 0, sizeof(port_chars));

    size_t j = 0;

    while (*c) {
        if ((PATH_DELIM     == c[0]) ||
            (QUERY_DELIM    == c[0]) ||
            (FRAGMENT_DELIM == c[0])) {
            break;
        } else if (!isdigit(c[0])) {
            fprintf(stderr,"port char '%c' is not a digit\n",c[0]);
            return ERROR_PORT;
        } else if (max_port_chars_len == j) {
            fprintf(stderr,"port str exceeds max port str len %lu\n",max_port_chars_len);
            return ERROR_PORT;
        } else {
            port_chars[j] = c[0];
        }
        c++;
        j++;
    }

    // We didn't set port chars. This is not an error: a port is not required.
    if (0 == j) {
        return NO_PORT;
    }

    // Convert the port chars to a long
    long port_long = (long) strtol(port_chars,NULL,10);
    if (0 >= port_long) {
        fprintf(stderr,"zero port or cannot convert port_chars %s to long\n",port_chars);
        return ERROR_PORT;
    }

    if (65535 <= port_long) {
        fprintf(stderr,"%lu exceeds the maximum port value\n",port_long);
        return ERROR_PORT;
    }

    // Advance pointer past the port.
    *s = c;

    *err_out = NO_UPARSE_ERROR;
    return (int) port_long;
}


// -----------------------------------------
// PATH PARSING

// A url does not need to have a path, so this can return NULL without an error
// being thrown

char *get_path(char **s, unsigned int *err_out) {

    *err_out = UPARSE_ERROR;

    if (NULL == *s) {
        return NULL;
    }

    // If the string is empty, that means there was nothing after
    // the host/port (no path). Return the vacuous path "/".
    if (strlen(*s) == 0) {
        *err_out = NO_UPARSE_ERROR;
        return strdup("/");
    }

    // Local copy we can advance.
    char *c = *s;

    // If the first char is not '/', then there is an error (our string is nonempty here).
    if (PATH_DELIM != c[0]) {
        return NULL;
    }

    // Advance past the '/'
    c++;

    // Choose a sensible limit for a path string.
    size_t const max_path_len = 1024;
    char path[max_path_len+1];
    memset(&path[0], 0, sizeof(path));

    size_t j = 0;

    // we designate '/' to be the vacuous path (every url has at least this).
    path[j] = PATH_DELIM;
    j++;
    
    while (*c) {
        if ((QUERY_DELIM    == c[0]) ||
            (FRAGMENT_DELIM == c[0])) {
            break;
        } else if (!(isalnum(c[0]) || (PATH_DELIM == c[0]))) {
            fprintf(stderr,"path char '%c' is not a alphanumeric or /\n",c[0]);
            return NULL;
        } else if (max_path_len == j) {
            fprintf(stderr,"path str exceeds max path str len %lu\n",max_path_len);
            return NULL;
        } else {
            path[j] = c[0];
        }
        c++;
        j++;
    }

    // Advance pointer past the path.
    *s = c;
    path[j] = '\0';
    *err_out = NO_UPARSE_ERROR;
    return strdup(path);
}


// -----------------------------------------
// QUERY PARSING

// A url doesn't have to have a ?query arg list, so this can return NULL
// and not be an error.

char *get_query(char **s, unsigned int *err_out) {

    *err_out = UPARSE_ERROR;

    if (NULL == *s) {
        *err_out = NO_UPARSE_ERROR;
        return NULL;
    }

    // If the string is empty, that means there was nothing after
    // the path (no query).
    if (strlen(*s) == 0) {
        *err_out = NO_UPARSE_ERROR;
        return NULL;
    }

    // Local copy we can advance.
    char *c = *s;

    // If the first char is not '?', then there is an error (our string is nonempty here).
    if (QUERY_DELIM != c[0]) {
        return NULL;
    }

    // Advance past the '?'
    c++;

    // Choose a sensible limit for a query string.
    size_t const max_query_len = 1024;
    char query[max_query_len+1];
    memset(&query[0], 0, sizeof(query));

    size_t j = 0;

    while (*c) {
        if (FRAGMENT_DELIM == c[0]) {
            break;
        } else if (!(isalnum(c[0]) || (QUERY_PAIR_DELIM == c[0]) || (QUERY_KEY_VAL_DELIM == c[0]))) {
            fprintf(stderr,"query char '%c' is not a alphanumeric or =\n",c[0]);
            return NULL;
        } else if (max_query_len == j) {
            fprintf(stderr,"query str exceeds max query str len %lu\n",max_query_len);
            return NULL;
        } else {
            query[j] = c[0];
        }
        c++;
        j++;
    }

    // Advance pointer past the query.
    *s = c;
    query[j] = '\0';
    *err_out = NO_UPARSE_ERROR;
    return strdup(query);
}


// -----------------------------------------
// FRAGMENT PARSING

// No special destructor needed, fragment is just char *. A url does not need to have
// a fragment, so a NULL return value is not strictly an error.

char *get_fragment(char **s, unsigned int *err_out) {

    *err_out = UPARSE_ERROR;

    if (NULL == *s) {
        *err_out = NO_UPARSE_ERROR;
        return NULL;
    }

    // If the string is empty, that means there was nothing after
    // the path/query (no fragment).
    if (strlen(*s) == 0) {
        *err_out = NO_UPARSE_ERROR;
        return NULL;
    }

    // Local copy we can advance.
    char *c = *s;

    // If the first char is not '#', then there is an error (our string is nonempty here).
    if (FRAGMENT_DELIM != c[0]) {
        return NULL;
    }

    // Advance past the '?'
    c++;

    // Choose a sensible limit for a fragment string.
    size_t const max_fragment_len = 1024;
    char fragment[max_fragment_len+1];
    memset(&fragment[0], 0, sizeof(fragment));

    size_t j = 0;

    while (*c) {
        if (!isalnum(c[0])) {
            fprintf(stderr,"fragment char '%c' is not a alphanumeric or =\n",c[0]);
            return NULL;
        } else if (max_fragment_len == j) {
            fprintf(stderr,"fragment str exceeds max fragment str len %lu\n",max_fragment_len);
            return NULL;
        } else {
            fragment[j] = c[0];
        }
        c++;
        j++;
    }

    // Advance pointer past the fragment.
    *s = c;
    fragment[j] = '\0';
    *err_out = NO_UPARSE_ERROR;
    return strdup(fragment);
}


// -----------------------------------------
// URL PARSING

// main function for parsing a string url into a url struct
url_t *parse_url(char const *const url_string,unsigned int *err_out) {

    *err_out = NO_UPARSE_ERROR;

    url_t *url = (url_t *) malloc(sizeof(url_t));
    if (NULL == url) {
        fprintf(stderr,"cannot allocate url\n");
        return NULL;
    }
    init_url_t(url);

    // get a mutable pointer to the url string, and set a copy that can be freed
    char *s = strdup(url_string);
    char *const free_s = s;

    char *scheme = get_protocol_scheme(&s,err_out);
    if (NULL == scheme) {
        fprintf(stderr,"cannot get scheme from %s\n",url_string);
        free_url_t(url);
        free(free_s);
        return NULL;
    }
    if (NO_UPARSE_ERROR != *err_out) {
        fprintf(stderr,"cannot get scheme from %s\n",url_string);
        free_url_t(url);
        free(free_s);
        free(scheme);
        return NULL;
    }
    url->scheme = scheme;

    char *host = get_host(&s,err_out);
    if (NULL == host) {
        fprintf(stderr,"cannot get host from %s\n",url_string);
        free_url_t(url);
        free(free_s);
        return NULL;
    }
    if (NO_UPARSE_ERROR != *err_out) {
        fprintf(stderr,"cannot get host from %s\n",url_string);
        free_url_t(url);
        free(free_s);
        free(host);
        return NULL;
    }
    url->host = host;

    int const port = get_port(&s,err_out);
    if (ERROR_PORT == port) {
        fprintf(stderr,"cannot get port from %s\n",url_string);
        free_url_t(url);
        free(free_s);
        return NULL;
    }
    if (NO_UPARSE_ERROR != *err_out) {
        fprintf(stderr,"cannot get port from %s\n",url_string);
        free_url_t(url);
        free(free_s);
        return NULL;
    }
    url->port = port;

    char *path = get_path(&s,err_out);
    if (NULL == path) {
        fprintf(stderr,"cannot get path from %s\n",url_string);
        free_url_t(url);
        free(free_s);
        return NULL;
    }
    if (NO_UPARSE_ERROR != *err_out) {
        fprintf(stderr,"cannot get path from %s\n",url_string);
        free_url_t(url);
        free(free_s);
        free(path);
        return NULL;
    }
    url->path = path;

    char *query = get_query(&s,err_out);
    if ((NULL != query) && (UPARSE_ERROR != *err_out)) {
        url->query = query;
    }
    
    char *fragment = get_fragment(&s,err_out);
    if ((NULL != fragment) && (UPARSE_ERROR != *err_out)) {
        url->fragment = fragment;
    }
    
    free(free_s);
    return url;
}

void print_url(url_t *u) {
    if (NULL == u) {
        printf("(null)\n");
        return;
    }
    printf(" [ %s ] :// [ %s ] : [ %u ] [ %s ] ",u->scheme,u->host,u->port,u->path);
    if (NULL != u->query) {
        printf("? [ %s ] ",u->query);
    }
    if (NULL != u->fragment) {
        printf("# [ %s ] ",u->fragment);
    }
    printf("\n");
    return;
}
