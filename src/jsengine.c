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
    JSInteger, JSPlus, JSMinus, JSMul, JSDiv, JSNewline, JSTokCount,
} JSTokType;

struct JSToken {
    JSTokType ttype;
    size_t val; // what this should be casted to depends on `ttype`
};

const char* tok_str_map[] = {
    [JSInteger] = "Integer",
    [JSPlus   ] = "Plus"   ,
    [JSMinus  ] = "Minus"  ,
    [JSMul    ] = "Mul"    ,
    [JSDiv    ] = "Div"    ,
    [JSNewline] = "Integer",
};
void print_token(JSToken tok) {
    if (tok.ttype >= JSTokCount) {
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
        switch (*content) {
        case ' ':
            continue;
        case '+':
            da_push(toks, ((JSToken) { .ttype=JSPlus }));
            continue;
        case '-':
            da_push(toks, ((JSToken) { .ttype=JSMinus }));
            continue;
        case '*':
            da_push(toks, ((JSToken) { .ttype=JSMul }));
            continue;
        case '/':
            da_push(toks, ((JSToken) { .ttype=JSDiv }));
            continue;
        case '\n':
            da_push(toks, ((JSToken) { .ttype=JSNewline }));
            continue;
        };
        if (isdigit(*content)) {
            char* start = content;
            for (; isdigit(*content); content++);
            char* end = content;
            char* numstr = strndup(start, (int) (end - start));
            int64_t num = strtoll(numstr, NULL, 10);
            free(numstr);
            da_push(toks, ((JSToken) { .ttype=JSInteger, .val=(size_t)num }));
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
