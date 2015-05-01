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
    return EXIT_SUCCESS;
}

int main(void) {

    char *url_str[] =
        {
            "https://foo.bar.com:512/foo/bar/baz?a=bbb&c=ddddd#boom",
            "https://foo.bar.com:512/foo/bar/baz",
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
