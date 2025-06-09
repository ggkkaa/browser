#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <miscutils.h>
#include <stddef.h>
#include <darray.h>
#include <jsengine.h>

const char* tok_str_map[] = {
    [JS_TOK_INTEGER] = "Integer",
};
void print_token(JSToken tok) {
    if (tok.ttype < 256) {
        if (tok.ttype == '\n') printf("(Newline)");
        else printf("(%c)", tok.ttype);
    } else if (tok.ttype >= JS_TOK_COUNT || !tok.ttype) {
        printf("(InvalidToken)");
    } else printf("(%s: %zu)", tok_str_map[tok.ttype], tok.val);
}

void dump_tokens(JSTokens toks) {
    printf("Token dump:\n");
    for (size_t i = 0; i < toks.len; i++) {
        print_token(toks.items[i]);
        if (i != toks.len - 1) printf(", ");
    }
    printf("\n");
}

int tokenise_js(JSTokens* toks, char* content) {
    for (; *content; content++) {
        if (*content == ' ') continue;
        else if (strchr("+-*/\n", *content)) {
            da_push(toks, ((JSToken) { .ttype=*content, .val=0 }));
        } else if (isdigit(*content)) {
            char* start = content;
            for (; isdigit(*content); content++);
            char* end = content;
            char* numstr = strndup(start, (int) (end - start));
            int64_t num = strtoll(numstr, NULL, 10);
            free(numstr);
            da_push(toks, ((JSToken) { .ttype=JS_TOK_INTEGER, .val=(size_t)num }));
        } else {
            fprintf(stderr, "invalid JS token: %d\n", *content);
            return -1;
        }
    }
    printf("Tokenising complete.\n");
    return 0;
}
