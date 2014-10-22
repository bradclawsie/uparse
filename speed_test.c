#include <stdlib.h>
#include <stdio.h>
#include "uparse.h"

int main(void) {
    char const *const url_str = "https://foo.bar.com:512/foo/bar/baz?a=bbb&c=ddddd#boom";
    unsigned int fail_count = 0;
    unsigned int url_out_err = 0;
    for (size_t i = 0; i < 1000000; i++) {
        url_out_err = NO_UPARSE_ERROR;
        url_t *url = parse_url(url_str,&url_out_err);
        if ((NULL == url) || (NO_UPARSE_ERROR != url_out_err)) {
            fail_count++;
        }
        free_url_t(url);
        url = NULL;
    }
}
