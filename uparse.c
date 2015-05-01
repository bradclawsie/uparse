#include "uparse.h"

// static unsigned int const MAX_PORT        = 65535;

static char const SCHEME_DELIM_PREFIX     = ':';
static char const SCHEME_SLASH            = '/';
/* static char const PATH_DELIM              = '/'; */
/* static char const HOST_PORT_DELIM         = ':'; */
/* static char const QUERY_DELIM             = '?'; */
/* static char const QUERY_PAIR_DELIM        = '&'; */
/* static char const QUERY_KEY_VAL_DELIM     = '='; */
/* static char const FRAGMENT_DELIM          = '#'; */

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

/* static bool is_unreserved(char c) { */
/*     return (isalnum(c) || ('.' == c) || ('_' == c) || ('-' == c) || ('~' == c)); */
/* } */

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

// Get the scheme, and advance past the expected ://
// Every url has a scheme. If this returns NULL, it is an error

static char *get_scheme(char **s, unsigned int *scheme_out_err) {

    *scheme_out_err = UPARSE_ERROR;

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
            scheme[j] = *c;
        }
        c++;
        j++;
    }

    if (!seen_prefix) {
        fprintf(stderr,"no scheme delimiter prefix '%c' found\n",SCHEME_DELIM_PREFIX);
        return NULL;
    }

    // If j is 0 here, then no alpha char was seen to copy to scheme[].
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
    return strdup(scheme);
}

// -----------------------------------------
// HOST:PORT PARSING

// Get the host:port section of the url. Doesn't support ipv6, unicode hosts,
// username annotations etc.
// Every url must have a host (but the port is optional). If this returns NULL,
// it is an error.

// -----------------------------------------
// PATH PARSING

// A url does not need to have a path, so this can return NULL without an error
// being thrown

// -----------------------------------------
// QUERY PARSING

// A url doesn't have to have a ?query arg list, so this can return NULL
// and not be an error.

// -----------------------------------------
// FRAGMENT PARSING

// No special destructor needed, fragment is just char *. A url does not need to have
// a fragment, so a NULL return value is not strictly an error.

// -----------------------------------------
// URL PARSING

// main function for parsing a string url into a url struct
url_t *parse_url(char const *const url_string,unsigned int *url_err_out) {

    *url_err_out = NO_UPARSE_ERROR;

    url_t *url = (url_t *) malloc(sizeof(url_t));
    if (NULL == url) {
        fprintf(stderr,"cannot allocate url\n");
        return NULL;
    }
    init_url_t(url);

    // get a mutable pointer to the url string, and set a copy that can be freed
    char *s = strdup(url_string);
    char *const free_s = s;

    char *scheme = get_scheme(&s,url_err_out);
    if (NULL == scheme) {
        fprintf(stderr,"cannot get scheme from %s\n",url_string);
        free_url_t(url);
        free(free_s);
        return NULL;
    }
    url->scheme = scheme;
    printf("have scheme:%s\n",scheme);

    free(free_s);
    return url;
}
