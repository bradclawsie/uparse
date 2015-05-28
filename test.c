#include <stdlib.h>
#include <stdio.h>
#include "uparse.h"

int test_url(const char *const url_str) {

    unsigned int url_out_err = 0;
    url_t *url = parse_url(url_str,&url_out_err);
    if ((NULL == url) || (NO_UPARSE_ERROR != url_out_err)) {
        fprintf(stderr,"bad retval from parse_url\n");
        if (NULL != url) {
            free_url_t(url);
            url = NULL;
        }
        return EXIT_FAILURE;
    }
    print_url(url);
    free_url_t(url);
    return EXIT_SUCCESS;
}

int main(void) {

    char *url_str[] =
        {
            "https://foo.bar.com:512/foo/bar/baz?a=bbb&c=ddddd#boom",
            "https://foo.bar.com:512/foo/bar/baz",
            "http://foo.com",
            "http://foo.com:43534534534",
            "http://foo.com:444fff666",
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

    char *arg_str[] = {
        "a=b&c=d", //ok
        "aaa=bbb&ccc=ddd", //ok
        "a=b", //ok
        "a", //bad
        "a+b", //bad
        "a=&b=c", //bad
        "a=b&&c=d", //bad
    };

    len = sizeof(arg_str)/sizeof(char *);
    char *qs;
    for (size_t i = 0; i < len; i++) {
        qs = arg_str[i];
        printf("%s\n",qs);
        unsigned int err_out = NO_UPARSE_ERROR;
        query_arg_list_t *r = get_query_arg_list(qs,&err_out);
        if (NULL == r) {
            printf("is null\n");
            if (NO_UPARSE_ERROR != err_out) {
                printf("there was an error\n");
            }
        } else {
            printf("%lu\n",r->count);
            size_t i = 0;
            for (i = 0; i < r->count; i++) {
                printf("%s -> %s\n",r->query_key_vals[i]->key,r->query_key_vals[i]->val);
            }
        }
        free_arg_list_t(r);
    }
}
