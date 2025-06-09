#pragma once
#include <stddef.h>
#include <stdint.h>

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

typedef enum {
    AST_NODE_BINOP,
    AST_NODE_UNSIGNED_INT,
} ASTNodeType;

typedef struct ASTBranch ASTBranch;
struct ASTBranch {
    ASTNodeType type;
    union {
        struct {
            ASTBranch *val1;
            int op; // ascii for the symbol
            ASTBranch *val2;
        } BinOpNode;
        struct {
            uint64_t val;
        } UnsignedInteger;
    };
};

int tokenise_js(JSTokens* toks, char* content);
int gen_ast(JSTokens toks, ASTBranch *ast);
void dump_tokens(JSTokens toks);
int run_js(char* content);
void dump_ast(ASTBranch *ast, int depth);
