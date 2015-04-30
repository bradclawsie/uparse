#include <stdlib.h>
#include <stdio.h>
#include "uparse.h"

int test_url(const char *const url_str) {

    unsigned int url_out_err = 0;
    url_t *url = parse_url(url_str,&url_out_err);
    if ((NULL == url) || (NO_UPARSE_ERROR != url_out_err)) {
        fprintf(stderr,"bad retval from parse_url\n");
        return EXIT_FAILURE;
    }

    // print
    printf("scheme:%s\n",url->scheme);
    printf("host:%s,port:%d\n",url->host,url->port);
    if (NULL != url->path_elt_list) {
        printf("path str %s\n",url->path_elt_list->path_str);
        printf("path count: %ld\n",url->path_elt_list->count);
        char **k = url->path_elt_list->path_elts;
        for (size_t i = 0; i < url->path_elt_list->count; i++) {
            printf("path elt %ld:\t%s\n",i,*k++);
        }
    } else {
        printf("path null\n");
    }
    if (NULL != url->query_arg_list) {
        for (size_t i = 0; i < url->query_arg_list->count; i++) {
            query_key_val_t *t = url->query_arg_list->query_key_vals[i];
            printf("key-val pair %ld:\t%s %s\n",i,t->key,t->val);
        }
    }
    if (NULL != url->fragment) {
        printf("fragment:%s\n",url->fragment);
    }

    // free
    free_url_t(url);
    url = NULL;

    return EXIT_SUCCESS;
}

int main(void) {

    char *url_str[] =
        {
            "https://foo.bar.com:512/foo/bar/baz?a=bbb&c=ddddd#boom",
            "http://foo.com",
            "http:// foo bar/",
            "http://foo.com}",
            "http//",
            "sftp:/|",
            "http://my.domain:badport",
            "http://my.domain:",
            "://my.domain",
            "https://foo.bar.com:512/foo/bar/baz ?a=bbb&c=ddddd#boom",
            "https://foo.bar.com:512/foo/bar/baz?abbb&c=ddddd#boom",
            "https://foo.bar.com:512/foo/bar/baz|a=bbb&c=ddddd#boom",
            "https://foo.bar.com:512/foo/bar/baz a=bbb&c=ddddd#boom",
            "https://foo.bar.com:512/foo/bar/baz?a=!bbb&c=ddddd#boom",
            "https://foo.bar.com:512/foo/bar/baz?a=bbb&c=ddddd#bo om",
            " http://foo.com",
            "https://foo.bar.com:512/?u=1234",
            "https://foo.bar.com:512?u=1234",
            "https://foo.bar.com:hi",
        };

    size_t len = sizeof(url_str)/sizeof(char *);
    for (size_t i = 0; i < len; i++) {
        printf("---------------------------------------\n");
        printf("%s\n",url_str[i]);
        int parsed_url = test_url(url_str[i]);
        if (EXIT_SUCCESS != parsed_url) {
            fprintf(stderr,"failure on %s\n",url_str[i]);
        }
    }

    char *esc_result1 = url_escape("hello!##there");
    char *esc_result2 = url_escape("!!!##");
    printf("|%s|\n",esc_result1);
    printf("|%s|\n",esc_result2);
    free(esc_result1);
    free(esc_result2);
}
