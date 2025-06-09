#include <jsengine.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define CHAR_OPS "*/+-"
int operation_precedence[] = {
    ['*'] = 2,
    ['/'] = 2,
    ['+'] = 1,
    ['-'] = 1,
};

void dump_ast(ASTBranch *ast, int depth) {
    switch (ast->type) {
    case AST_NODE_UNSIGNED_INT:
        printf("%.*s%zu\n", depth, " ", ast->UnsignedInteger.val);
        break;
    default:
        printf("%.*sInvalid branch\n", depth, " ");
        break;
    };
}

#define INIT_TOK_PREC 1000 // there should really never actually be an operation precedence
                           // this high
int gen_ast(JSTokens toks, ASTBranch *ast) {
    // go through and find the operation with the lowest precedence
    size_t min_tok_precedence = INIT_TOK_PREC, min_tok_idx;
    for (size_t i = 0; i < toks.len; i++) {
        if (toks.items[i].ttype >= 256 || !strchr(CHAR_OPS, toks.items[i].ttype)) continue;
        size_t this_precedence = operation_precedence[toks.items[i].ttype]; 
        if (this_precedence >= min_tok_precedence) continue;
        min_tok_precedence = this_precedence;
        min_tok_idx = i;
    }
    if (min_tok_precedence == INIT_TOK_PREC) {
        if (toks.len != 1 || toks.items[0].ttype != JS_TOK_INTEGER) {
            fprintf(stderr, "invalid syntax (possibly todo for allowed lone types? idk)\n");
            return -1;
        }
        *ast = (ASTBranch) {
            .type = AST_NODE_UNSIGNED_INT,
            .UnsignedInteger = { .val=toks.items[0].val },
        };
        return 0;
    } else {
        if (toks.items[min_tok_idx].ttype >= 256 ||
                !operation_precedence[toks.items[min_tok_idx].ttype]) {
            fprintf(stderr, "Invalid operation: %d\n", toks.items[min_tok_idx].ttype);
            return -1;
        }
        // this assumes it's a BinOpNode, this should be fixed (TODO)
        if (min_tok_idx < 1 || min_tok_idx >= toks.len) {
            fprintf(stderr, "invalid syntax around operation\n");
            return -1;
        }
        JSTokens toks_before = toks;
        JSTokens toks_after  = toks;
        toks_before.len = min_tok_idx;
        toks_after.items += min_tok_idx + 1;
        toks_after.len -= min_tok_idx;
        ast->BinOpNode.val1 = (ASTBranch*) malloc(sizeof(ASTBranch));
        ast->BinOpNode.val2 = (ASTBranch*) malloc(sizeof(ASTBranch));
        gen_ast(toks_before, ast->BinOpNode.val1);
        gen_ast(toks_after, ast->BinOpNode.val2);
        ast->BinOpNode.op = toks.items[min_tok_idx].ttype;
        ast->type = AST_NODE_BINOP;
    }
    return 0;
}
