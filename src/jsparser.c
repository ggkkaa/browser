#include <jsengine.h>
#include <stddef.h>
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

#define PRINT_SPACES(x) \
    do { \
        for (int si = 0; si < x; si++) printf(" "); \
    } while (0)
void dump_ast(ASTBranch *ast, int depth) {
    switch (ast->type) {
    case AST_NODE_UNSIGNED_INT:
        PRINT_SPACES(depth); printf("-> UnsignedInteger(%zu)\n", ast->UnsignedInteger.val);
        break;
    case AST_NODE_BINOP:
        PRINT_SPACES(depth); printf("-> BinOpNode(%c):\n", ast->BinOpNode.op);
        dump_ast(ast->BinOpNode.val1, depth + 4);
        dump_ast(ast->BinOpNode.val2, depth + 4);
        break;
    default:
        PRINT_SPACES(depth); printf("-> Invalid node\n");
        break;
    };
}

#define INIT_TOK_PREC 1000 // there should really never actually be an operation precedence
                           // this high
int gen_ast(JSTokens toks, ASTBranch *ast) {
    // go through and find the operation with the lowest precedence
    size_t min_tok_precedence = INIT_TOK_PREC;
    size_t min_tok_idx=0, min_tok_paren_depth=0;
    size_t paren_depth = min_tok_paren_depth = 0;
    for (size_t i = 0; i < toks.len; i++) {
        if      (toks.items[i].ttype == '(') paren_depth++;
        else if (toks.items[i].ttype == ')') {
            if (!paren_depth) {
                fprintf(stderr, "invalid syntax (tried to use right parenthesis with no parenthesis open\n");
                return -1;
            }
            paren_depth--;
        }
        if (toks.items[i].ttype >= 256 || !strchr(CHAR_OPS, toks.items[i].ttype)) continue;
        size_t this_precedence = operation_precedence[toks.items[i].ttype];
        if (min_tok_precedence < INIT_TOK_PREC && min_tok_paren_depth < paren_depth) continue;
        if (this_precedence > min_tok_precedence || paren_depth < min_tok_paren_depth) continue;
        min_tok_precedence = this_precedence;
        min_tok_idx = i;
        min_tok_paren_depth = paren_depth;
    }
    if (min_tok_precedence == INIT_TOK_PREC) {
        for (size_t i = 0; i < toks.len; i++) {
            if (toks.items[i].ttype == '(' || toks.items[i].ttype == ')') continue;
            else if (toks.items[i].ttype == JS_TOK_INTEGER) {
                *ast = (ASTBranch) {
                    .type = AST_NODE_UNSIGNED_INT,
                    .UnsignedInteger = { .val=toks.items[i].val },
                };
                return 0;
            } else {
                fprintf(stderr, "invalid syntax (possibly todo for allowed one types? idk)\n");
                return -1;
            }
        }
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
        toks_after.len -= min_tok_idx + 1;
        ast->BinOpNode.val1 = (ASTBranch*) malloc(sizeof(ASTBranch));
        ast->BinOpNode.val2 = (ASTBranch*) malloc(sizeof(ASTBranch));
        if (gen_ast(toks_before, ast->BinOpNode.val1) < 0) return -1;
        if (gen_ast(toks_after, ast->BinOpNode.val2) < 0) return -1;
        ast->BinOpNode.op = toks.items[min_tok_idx].ttype;
        ast->type = AST_NODE_BINOP;
    }
    return 0;
}
