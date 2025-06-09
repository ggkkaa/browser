#include <jsengine.h>

int operation_precendence[] = {
    ['*'] = 2,
    ['/'] = 2,
    ['+'] = 1,
    ['-'] = 1,
};

#define INIT_TOK_PREC 1000 // there should really never actually be an operation precedence
                           // this high
int gen_ast(JSTokens toks, ASTBranch *ast) {
    (void) toks, (void) ast;
    // go through and find the operation with the lowest precendence
    size_t min_tok_precedence = INIT_TOK_PREC, min_tok_idx;
    for (size_t i = 0; i < toks.len; i++) {
        size_t this_precedence = operation_precendence[toks.items[i]]; 
        if (operation_precedence >= min_tok_precendence) continue;
        min_tok_precedence = operation_precedence;
        min_tok_idx = i;
    }
    if (min_tok_precedence == INIT_TOK_PREC) {
        if (toks.len != 1 || toks.items[0].ttype != JS_TOK_INTEGER) {
            fprintf(stderr, "invalid syntax (possibly todo for allowed lone types? idk)\n");
            return -1;
        }
        *ast = (ASTBranch) {
            .type = JS_TOK_UNSIGNED_INT,
            .UnsignedInteger = { .val=toks.items[0].val },
        };
        return 0;
    } else {
        if (toks.items[min_tok_idx].val > 256 ||
                !operation_precedence[toks.items[min_tok_idx].val]) {
            fprintf(stderr, "Invalid operation\n");
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
        gen_ast(toks_before, ast->val1);
        gen_ast(toks_after, ast->val2);
        ast->op = toks.items[min_tok_idx];
        ast->type = AST_NODE_BINOP;
    }
    return 0;
}
