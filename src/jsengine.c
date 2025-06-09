// Pedal: the ECMAScript bytecode compiler + interpreter for Bikeshed
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <miscutils.h>
#include <stddef.h>
#include <darray.h>

typedef struct JSToken JSToken;
typedef struct {
    JSToken* items;
    size_t len, cap;
} JSTokens;

typedef enum {
    JS_TOK_NEWLINE='\n',
    JS_TOK_MUL    ='*',
    JS_TOK_PLUS   ='+',
    JS_TOK_MINUS  ='-',
    JS_TOK_DIV    ='/',
    JS_TOK_INTEGER,
    JS_TOK_COUNT  ,
} JSTokType;

struct JSToken {
    JSTokType ttype;
    size_t val; // what this should be casted to depends on `ttype`
};

const char* tok_str_map[] = {
    [JS_TOK_NEWLINE] = "Integer",
    [JS_TOK_MUL    ] = "Mul"    ,
    [JS_TOK_PLUS   ] = "Plus"   ,
    [JS_TOK_MINUS  ] = "Minus"  ,
    [JS_TOK_DIV    ] = "Div"    ,
    [JS_TOK_INTEGER] = "Integer",
};
void print_token(JSToken tok) {
    if (tok.ttype >= JS_TOK_COUNT || !tok.ttype) {
        printf("(InvalidToken)");
        return;
    }
    printf("(%s: %zu)", tok_str_map[tok.ttype], tok.val);
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
        char* single_char_toks = "+-*/\n ";
        if (strchr(single_char_toks, *content)) {
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

int run_js(char* content) {
    JSTokens toks = {0};
    if (tokenise_js(&toks, content) < 0) {
        fprintf(stderr, "Failed to tokenise JS\n");
        return -HTMLERR_INVALID_JS;
    }
    dump_tokens(toks);
    return 0;
}
