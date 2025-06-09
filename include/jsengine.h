#pragma once

typedef struct JSToken JSToken;
typedef struct {
    JSToken* items;
    size_t len, cap;
} JSTokens;

typedef enum {
    JS_TOK_INTEGER=256,
    JS_TOK_COUNT,
} JSTokType;

struct JSToken {
    JSTokType ttype;
    size_t val; // what this should be casted to depends on `ttype`
};

int tokenise_js(JSTokens* toks, char* content);
void dump_tokens(JSTokens toks);
int run_js(char* content);
