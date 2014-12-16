#include "uparse.h"

static unsigned int const MAX_PORT        = 65535;

static char const SCHEME_DELIM_PREFIX     = ':';
static char const SCHEME_SLASH            = '/';
static char const PATH_DELIM              = '/';
static char const HOST_PORT_DELIM         = ':';
static char const QUERY_DELIM             = '?';
static char const QUERY_PAIR_DELIM        = '&';
static char const QUERY_KEY_VAL_DELIM     = '=';
static char const FRAGMENT_DELIM          = '#';

#define ESCAPE_CHARS_COUNT 18


static char const ESCAPE_CHARS[ESCAPE_CHARS_COUNT] =
    {'!','#','$','&','\'','(',')','*','+',
     ',','/',':',';','=','?','@',']','['};
static char const *const PERCENT_REPLACE[ESCAPE_CHARS_COUNT] =
    {"%21","%23","%24","%26","%27","%28","%29","%2A","%2B",
     "%2C","%2F","%3A","%3B","%3D","%3F","%40","%5B","%5D"};

// -----------------------------------------

// url escape a string. assumes all chars will need to be replaced,
// allocates enough space to do so.
char *url_escape(char const *const s) {
    char const *c = s;
    char *esc_s = (char *) calloc((3 * strlen(s)) + 1,sizeof(char));
    size_t j = 0;
    bool percent_replaced;
    while (*c) {
        percent_replaced = false;
        for (size_t i = 0; i < ESCAPE_CHARS_COUNT; i++) {
            if (ESCAPE_CHARS[i] == *c) {
                // *c needs to be replaced in esc_s with PERCENT_REPLACE[i]
                char const *t = PERCENT_REPLACE[i];
                for (size_t k = 0; k < 3; k++) {
                    esc_s[j++] = *t++;
                }
                percent_replaced = true;
                break;
            }
        }
        if (!percent_replaced) {
            // *c was not a escape char
            esc_s[j++] = *c;
        }
        c++;
    }
    esc_s[j] = '\0';
    return esc_s;
}

// -----------------------------------------

static bool is_unreserved(char c) {
    return (isalnum(c) || ('.' == c) || ('_' == c) || ('-' == c) || ('~' == c)); 
}

// -----------------------------------------

// a url is a { scheme, host_port, path, query, fragment }

// -----------------
// scheme is a char * and does not need a dedicated destructor.

// -----------------
// host_port
typedef struct host_port_t {
    char *host;
    unsigned int port;
} host_port_t;

// host_port destructor
static void free_host_port_t(host_port_t *host_port) {
    if (NULL == host_port) {
        return;
    }
    if (NULL != host_port->host) {
        free(host_port->host);
    }
    free(host_port);
}

// -----------------
// path

// path destructor
static void free_path_t(path_t *path) {
    if (NULL == path) {
        return;
    }
    if (NULL != path->path_str) {
        free(path->path_str);
    }
    for (size_t i = 0; i < path->count; i++) {
        if (NULL != path->path_elts[i]) {
            free(path->path_elts[i]);
        }
    }
    if (NULL != path->path_elts) {
        free(path->path_elts);
    }
    free(path);
}

// -----------------
// query (key/val pairs)

// query_key_val destrctor
static void free_query_key_val_t(query_key_val_t *query_key_val) {
    if (NULL == query_key_val) {
        return;
    }
    if (NULL != query_key_val->key) {
        free(query_key_val->key);
    }
    if (NULL != query_key_val->val) {
        free(query_key_val->val);
    }
    free(query_key_val);
}

// query_key_val list destructor
static void free_query_key_val_t_list(query_key_val_t **query_key_vals,size_t len) { 
    if (NULL == query_key_vals) {
        return;
    }
    for (size_t i = 0; i < len; i++) {
        free_query_key_val_t(query_key_vals[i]);
    }
    free(query_key_vals);
}

// query_arg_list destructor
static void free_arg_list_t(query_arg_list_t *query_arg_list) {
    if (NULL == query_arg_list) {
        return;
    }
    free_query_key_val_t_list(query_arg_list->query_key_vals,query_arg_list->count);
    free(query_arg_list);
}

// -----------------
// fragment is a char * and does not need a dedicated destructor

// -----------------
// the public url struct

void init_url_t(url_t *url) {
    url->scheme         = NULL;
    url->host           = NULL;
    url->port           = 0;
    url->path_elt_list  = NULL;
    url->query_arg_list = NULL;
    url->fragment       = NULL;
    url->original       = NULL;
}

void free_url_t(url_t *url) {
    free(url->scheme);
    url->scheme = NULL;
    free(url->host);
    url->host = NULL;
    if (NULL != url->path_elt_list) {
        free_path_t(url->path_elt_list);
        url->path_elt_list = NULL;
    }
    if (NULL != url->query_arg_list) {
        free_arg_list_t(url->query_arg_list);
        url->query_arg_list = NULL;
    }
    free(url->fragment);
    url->fragment = NULL;
    free(url->original);
    url->original = NULL;
    free(url);
}

// -----------------------------------------
// SCHEME PARSING

// Get the scheme, and advance past the expected ://
// Every url has a scheme. If this returns NULL, it is an error
static char *get_scheme(char **s, unsigned int *scheme_out_err) {
    *scheme_out_err = UPARSE_ERROR; // default
    if ((NULL == s) || (0 == strlen(*s))) {
        fprintf(stderr,"arg pointer null\n");
        return NULL;
    }
    char *c = *s;
    bool seen_prefix = false;
    size_t scheme_len = 0;
    // read scheme
    while (*c) {
        if (SCHEME_DELIM_PREFIX == c[0]) {
            seen_prefix = true;
            c++; // advance past SCHEME_DELIM_PREFIX
            break;
        } else if (!isalpha(*c)) {
            fprintf(stderr,"'%c' is invalid\n",*c);
            return NULL;
        }
        scheme_len++;
        c++;
    }
    if (!seen_prefix) {
        fprintf(stderr,"scheme prefix %c not seen\n",SCHEME_DELIM_PREFIX);
        return NULL;
    }
    if (0 == scheme_len) {
        fprintf(stderr,"scheme is zero length\n");
        return NULL;
    }
    // parse slashes
    char *scheme = (char *) calloc(scheme_len+1,sizeof(char));
    if (NULL == scheme) {
        fprintf(stderr,"cannot allocate scheme\n");
        return NULL;
    }
    strlcpy(scheme,*s,scheme_len+1);

    for (size_t i = 0; i < 2; i++) {
        if ((NULL == c) || (SCHEME_SLASH != c[0])) {
            fprintf(stderr,"scheme slash %c not seen\n",SCHEME_SLASH);
            free(scheme);
            return NULL;
        }
        c++;
    }
    *s = c; // set pointer past scheme
    *scheme_out_err = NO_UPARSE_ERROR;
    return scheme;
}

// -----------------------------------------
// HOST:PORT PARSING

// Get the host:port section of the url. Doesn't support ipv6, unicode hosts, 
// username annotations etc.
// Every url must have a host (but the port is optional). If this returns NULL,
// it is an error.
static host_port_t *get_host_port(char **s, unsigned int *host_port_out_err) {
    *host_port_out_err = UPARSE_ERROR; // default
    if ((NULL == s) || (0 == strlen(*s))) {
        fprintf(stderr,"arg pointer null\n");
        return NULL;    
    }

    char *c = *s; 
    size_t host_len = 0;
    size_t port_len = 0;
    char *host_start = *s;
    char *port_start = NULL;
    bool seen_host_delim = false;
    while (*c) {
        // normally we would see the PATH_DELIM to break this, but 
        // this can be '', so we must also look out for the 
        // QUERY_DELIM
        if ((PATH_DELIM == c[0]) || (QUERY_DELIM == c[0])) {
            break;
        } else if (HOST_PORT_DELIM == c[0]) {
            seen_host_delim = true;
            c++;
            port_start = c;
        } else {
            // *c is part of the host or port, depending on delimiters seen
            if (!is_unreserved(*c)) {
                fprintf(stderr,"'%c' is invalid\n",*c);
                return NULL;
            }
            // If we have already seen the host/port delimiter (:), then
            // count the chars in the length of the port part. Otherwise,
            // count the chars in the length of the host part.
            seen_host_delim ? port_len++ : host_len++;
            c++;
        }
    }
    if (0 == host_len) {
        fprintf(stderr,"no host found\n");
        return NULL;
    }
    if (seen_host_delim && (0 == port_len)) {
        fprintf(stderr,"port delimiter seen but no port number string\n");
        return NULL;
    }

    host_port_t *host_port = (host_port_t *) malloc(sizeof(host_port_t));
    if (NULL == host_port) {
        fprintf(stderr,"cannot allocate host_port\n");
        return NULL;            
    }
    bzero((void *) host_port,sizeof(host_port_t));

    host_port->host = strndup(host_start,host_len);
    if (NULL == host_port->host) {
        fprintf(stderr,"cannot allocate host_port->host\n");
        free_host_port_t(host_port);
        return NULL;
    }

    host_port->port = 0;
    if ((seen_host_delim) && (port_len > 0)) {
        char *port_chars = strndup(port_start,port_len);
        if (NULL == port_chars) {
            fprintf(stderr,"cannot allocate chars for host_port->port\n");
            free_host_port_t(host_port);
            return NULL;
        }
        long port_long = (long) strtol(port_chars,NULL,10);
        if (0 == port_long) {
            fprintf(stderr,"zero port or cannot convert port_chars %s to long\n",port_chars);
            free_host_port_t(host_port);
            free(port_chars);
            return NULL;
        }
        free(port_chars);
        if (MAX_PORT < port_long) {
            fprintf(stderr,"port %ld out of range\n",port_long);
            free_host_port_t(host_port);
            *host_port_out_err = UPARSE_ERROR;
            return NULL;
        }
        host_port->port = (unsigned int) port_long;
    }

    *s = c; // set pointer past host/port
    *host_port_out_err = NO_UPARSE_ERROR;
    return host_port;
}

// -----------------------------------------
// PATH PARSING

// A url does not need to have a path, so this can return NULL without an error
// being thrown
static path_t *get_path(char **s, unsigned int *path_out_err) {
    if ((NULL == s) || (0 == strlen(*s))) {
        *path_out_err = NO_UPARSE_ERROR; // not an error: url can have no path
        return NULL;
    }

    *path_out_err = UPARSE_ERROR; // default until end of function

    size_t delim_count = 0;
    size_t chars_until_query_delim = 0;
    char *c = *s;
    while (*c) {
        if (PATH_DELIM == *c) {
            delim_count++;
        } else if (QUERY_DELIM == *c) {
            break;
        } else if (!is_unreserved(*c)) {
            fprintf(stderr,"'%c' is invalid\n",*c);
            return NULL;            
        }
        chars_until_query_delim++;
        c++;
    }

    char *just_path;
    const char path_delim_str[2] = {PATH_DELIM,'\0'}; // "/" (not '/')
    char nonempty_path[chars_until_query_delim+1];

    if (0 == delim_count) {
        // in this case, the path was '', not even '/'. so we create 
        // a default empty path of '/' to represent what '' implies
        just_path = (char *) &path_delim_str;
    } else {
        bzero((void *) nonempty_path,(chars_until_query_delim + 1) * sizeof(char));
        strlcpy(nonempty_path,*s,chars_until_query_delim+1);
        just_path = (char *) &nonempty_path;
    }

    path_t *path = (path_t *) malloc(sizeof(path_t));
    if (NULL == path) {
        fprintf(stderr,"cannot allocate path\n");
        return NULL;
    }
    bzero((void *) path,sizeof(path_t));
    path->path_str = strdup(just_path);

    // set pointer to where the query delim may be found, now that *s
    // has been copied into just_path
    *s = c;

    // case of single '/' path (like: http://foo.com/?key=val)
    if (strlen(just_path) == 1) {
        path->path_elts = (char **) malloc(1 * sizeof(char *));
        path->path_elts[0] = (char *) calloc(1,sizeof(char));
        path->count = 1;
        *path_out_err = NO_UPARSE_ERROR;
        return path;
    }

    path->path_elts = (char **) malloc(delim_count * sizeof(char *));
    
    if (NULL == path->path_elts) {
        fprintf(stderr,"cannot allocate path\n");
        return NULL;        
    }
    path->count = delim_count;

    char *tok;
    size_t i = 0;

    while (((tok = strsep(&just_path,path_delim_str)) != NULL) && (i < delim_count)) {
        if (0 != strcmp("",tok)) {
            path->path_elts[i] = strdup(tok);
            if (NULL == path->path_elts[i]) {
                fprintf(stderr,"cannot dup %s\n",tok);
                free_path_t(path);
                free(tok);                
                return NULL;
            }
            i++;
        }
    }

    *path_out_err = NO_UPARSE_ERROR;
    return path;
}

// -----------------------------------------
// QUERY PARSING

// A url doesn't have to have a ?query arg list, so this can return NULL
// and not be an error. 
static query_arg_list_t *get_query_arg_list(char **s,unsigned int *query_out_err) {
    if ((NULL == s) || (0 == strlen(*s))) {
        *query_out_err = NO_UPARSE_ERROR;
        return NULL;
    }
    char *c = *s;

    *query_out_err = UPARSE_ERROR; // default until end of function

    if (QUERY_DELIM != c[0]) {
        fprintf(stderr,"no %c as prefix\n",QUERY_DELIM);
        return NULL;
    }
    c++;
    if (NULL == c) {
        fprintf(stderr,"after %c found, no text\n",QUERY_DELIM);
        return NULL;
    }
    // at the end of the loop, t will point to the end of the query string, which
    // may not be the end of the url; it may have a #fragment
    char *t = c;
    unsigned int query_delim_count = 0;
    unsigned int query_pair_count = 0;
    unsigned int query_str_len = 0;
    while ((*t) && (FRAGMENT_DELIM != *t)) {
        if (QUERY_PAIR_DELIM == *t) {
            query_delim_count++;
        }
        t++;
        query_str_len++;
    }
    query_pair_count = query_delim_count + 1;

    char query_string[query_str_len+1];
    bzero((void *) query_string,(query_str_len+1) * sizeof(char));
    strlcpy(query_string,c,query_str_len+1);

    // stringify the delims for strsep
    char query_pair_delim_str[2];
    snprintf(query_pair_delim_str,2,"%c",QUERY_PAIR_DELIM);
    char query_key_value_delim_str[2];
    snprintf(query_key_value_delim_str,2,"%c",QUERY_KEY_VAL_DELIM);

    query_key_val_t **query_key_vals =
        (query_key_val_t **) malloc(query_pair_count * sizeof(query_key_val_t *));
    if (NULL == query_key_vals) {
        fprintf(stderr,"could not allocate query_key_val\n");
        return NULL;
    }
    bzero((void *) query_key_vals,query_pair_count * sizeof(query_key_val_t *));

    unsigned int i = 0;
    char *pair_tok;
    char *free_query_string = query_string;
    while ((pair_tok = strsep(&free_query_string,query_pair_delim_str)) != NULL) {
        if (0 != strcmp("",pair_tok)) {
            if (i >= query_pair_count) {
                fprintf(stderr,"loop count %d >= previous pair count %d\n",
                        i,query_pair_count);
                free_query_key_val_t_list(query_key_vals,query_pair_count);
                return NULL;
            }
            char *sep_pair_tok = strdup(pair_tok);
            if (NULL == sep_pair_tok) {
                fprintf(stderr,"could not allocate sep_pair_tok\n");
                free_query_key_val_t_list(query_key_vals,query_pair_count);
                return NULL;
            }
            char *free_sep_pair_tok = sep_pair_tok;

            // put the key and val in a struct, and set as item i in a list
            // query_pair_count long (which will need to be allocated above)
            query_key_val_t *query_key_val_tok =
                (query_key_val_t *) malloc(sizeof(query_key_val_t));
            if (NULL == query_key_val_tok) {
                fprintf(stderr,"could not allocate query_key_val_tok\n");
                free_query_key_val_t_list(query_key_vals,query_pair_count);
                free(free_sep_pair_tok);
                return NULL;
            }
            bzero((void *) query_key_val_tok,sizeof(query_key_val_t));

            bool seen_key = false;
            char *kv_tok = NULL;
            char *key = NULL;
            char *val = NULL;

            // make sure sep_pair_tok contains an = to delimit the key and value
            if (NULL == strstr(sep_pair_tok,query_key_value_delim_str)) {
                fprintf(stderr,"could not find '%s' in query pair '%s'\n",
                        query_key_value_delim_str,sep_pair_tok);
                free_query_key_val_t_list(query_key_vals,query_pair_count);
                free_query_key_val_t(query_key_val_tok);
                free(free_sep_pair_tok);
                return NULL;
            }

            // break sep_pair_tok (a=b) into the key (a) and the value (b)
            while ((kv_tok = strsep(&sep_pair_tok,query_key_value_delim_str)) != NULL) {
                if (!seen_key) {
                    key = strdup(kv_tok);
                    seen_key = true;
                } else {
                    val = strdup(kv_tok);
                }
            }
            free(kv_tok);

            if ((NULL == key) || (NULL == val)) {
                fprintf(stderr,"either key or val from %s was null\n",free_sep_pair_tok);
                free_query_key_val_t_list(query_key_vals,query_pair_count);               
                free_query_key_val_t(query_key_val_tok);
                free(free_sep_pair_tok);
                free(key);
                free(val);
                return NULL;
            }
            
            char *key_val_list[] = {key,val};
            char *test_kv;
            for (size_t j = 0;j < 2;j++) {
                test_kv = key_val_list[i];
                while (*test_kv) {
                    if (!is_unreserved(*test_kv)) {
                        fprintf(stderr,"'%c' is invalid\n",*test_kv);
                        free_query_key_val_t_list(query_key_vals,query_pair_count);
                        free(free_sep_pair_tok);
                        free_query_key_val_t(query_key_val_tok);
                        free(key);
                        free(val);
                        return NULL;
                    }
                    test_kv++;
                }
            }

            query_key_val_tok->key = key;
            query_key_val_tok->val = val;
            query_key_vals[i] = query_key_val_tok;
            free(free_sep_pair_tok);
            i++;
        }
    }

    free(pair_tok);

    query_arg_list_t *query_arg_list =
        (query_arg_list_t *) malloc(sizeof(query_arg_list_t));
    if (NULL == query_arg_list) {
        fprintf(stderr,"cannot allocate query_arg_list\n");
        return NULL;
    }
    bzero((void *) query_arg_list,sizeof(query_arg_list_t));

    query_arg_list->count = i;
    query_arg_list->query_key_vals = query_key_vals;
    *s = t; // pointer is now past query arg str, at # fragment delim if there
    *query_out_err = NO_UPARSE_ERROR;
    return query_arg_list;
}

// -----------------------------------------
// FRAGMENT PARSING

// No special destructor needed, fragment is just char *. A url does not need to have
// a fragment, so a NULL return value is not strictly an error.
static char *get_fragment(char **s,unsigned int *fragment_out_err) {
    *fragment_out_err = NO_UPARSE_ERROR;
    if ((NULL == s) || (0 == strlen(*s))) {
        return NULL;
    }

    char *c = *s;

    if (FRAGMENT_DELIM != c[0]) {
        fprintf(stderr,"no %c as prefix\n",FRAGMENT_DELIM);
        *fragment_out_err = UPARSE_ERROR;
        return NULL;
    }
    c++;

    size_t fragment_len = strlen(c);
    if (fragment_len == 0) {
        // no fragment
        *fragment_out_err = NO_UPARSE_ERROR;
        return NULL;
    }

    char *fragment = strdup(c);
    if (NULL == fragment) {
        fprintf(stderr,"cannot allocate fragment\n");
        *fragment_out_err = UPARSE_ERROR;
        return NULL;        
    }
    char *test_fragment = fragment;
    while (*test_fragment) {        
        if (!is_unreserved(*test_fragment)) {
            fprintf(stderr,"'%c' is invalid\n",*test_fragment);
            free(fragment);
            *fragment_out_err = UPARSE_ERROR;
            return NULL;
        }
        test_fragment++;
    }

    *fragment_out_err = NO_UPARSE_ERROR;
    return fragment;
}

// -----------------------------------------
// URL PARSING

// main function for parsing a string url into a url struct
url_t *parse_url(char const *const url_string,unsigned int *url_err_out) {
    *url_err_out = UPARSE_ERROR;

    url_t *url = (url_t *) malloc(sizeof(url_t));
    if (NULL == url) {
        fprintf(stderr,"cannot allocate url\n");
        return NULL;
    }
    init_url_t(url);

    url->original = strdup(url_string);
    if (NULL == url->original) {
        fprintf(stderr,"cannot allocate original\n");
        free_url_t(url);
        return NULL;
    }

    char *mut_url_string = strdup(url_string);
    char *free_mut_url_string = mut_url_string;
    if (NULL == mut_url_string) {
        fprintf(stderr,"cannot allocate url\n");
        free_url_t(url);
        return NULL;
    }

    unsigned int scheme_out_err = 0;
    url->scheme = get_scheme(&mut_url_string,&scheme_out_err);
    if (NO_UPARSE_ERROR != scheme_out_err) {
        fprintf(stderr,"fail from get_scheme\n");
        free_url_t(url);
        free(free_mut_url_string);
        return NULL;
    }

    unsigned int host_port_out_err = 0;
    host_port_t *host_port = get_host_port(&mut_url_string,&host_port_out_err);
    if (NO_UPARSE_ERROR != host_port_out_err) {
        fprintf(stderr,"fail from get_host_port\n");
        free_url_t(url);
        free(free_mut_url_string);
        return NULL;
    }
    url->host = strdup(host_port->host);
    if (NULL == url->host) {
        fprintf(stderr,"cannot allocate host\n");
        free_url_t(url);
        free(free_mut_url_string);
        return NULL;        
    }
    url->port = host_port->port;
    free_host_port_t(host_port);

    unsigned int path_err_out = 0;
    url->path_elt_list = get_path(&mut_url_string,&path_err_out);
    if (NO_UPARSE_ERROR != path_err_out) {
        fprintf(stderr,"fail from get_path\n");
        free_url_t(url);
        free(free_mut_url_string);
        return NULL;
    }

    unsigned int query_err_out = 0;
    url->query_arg_list = get_query_arg_list(&mut_url_string,&query_err_out);
    if (UPARSE_ERROR == query_err_out) {
        fprintf(stderr,"fail from get_query_arg_list\n");
        free_url_t(url);
        free(free_mut_url_string);
        return NULL;
    }

    unsigned int fragment_err_out = 0;
    url->fragment = get_fragment(&mut_url_string,&fragment_err_out);
    if (UPARSE_ERROR == fragment_err_out) {
        fprintf(stderr,"fail from get_fragment\n");
        free_url_t(url);
        free(free_mut_url_string);
        return NULL;
    }

    free(free_mut_url_string);
    *url_err_out = NO_UPARSE_ERROR;
    return url;
}

