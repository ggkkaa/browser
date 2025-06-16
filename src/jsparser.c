#include <jsengine.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define CHAR_OPS "=*/+-!"
#define BINOPS   "=*/+-"

int operation_precedence[] = {
    ['!'] = 4,
    ['*'] = 3,
    ['/'] = 3,
    ['+'] = 2,
    ['-'] = 2,
    ['='] = 1,
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
    case AST_NODE_IDENT:
        PRINT_SPACES(depth); printf("-> Identifier(%s)\n", (char*) ast->UnsignedInteger.val);
        break;
    case AST_NODE_BINOP:
        PRINT_SPACES(depth); printf("-> BinOpNode(%c):\n", ast->BinOpNode.op);
        dump_ast(ast->BinOpNode.val1, depth + 4);
        dump_ast(ast->BinOpNode.val2, depth + 4);
        break;
    case AST_NODE_UNARYOP:
        PRINT_SPACES(depth); printf("-> UnaryOpNode(%c):\n", ast->UnaryOp.op);
        dump_ast(ast->UnaryOp.val, depth + 4);
        break;
    default:
        PRINT_SPACES(depth); printf("-> Invalid node\n");
        break;
    };
}

static ssize_t find_xth_tok(size_t x, JSTokType tok, JSTokens toks) {
    size_t count = 0;
    for (size_t i = 0; i < toks.len; i++) {
        if (toks.items[i].ttype != tok) continue;
        if (count == x) return i;
        count++;
    }
    return -1;
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
                fprintf(stderr, "invalid syntax (tried to use right parenthesis with no parenthesis open)\n");
                return -1;
            }
            paren_depth--;
        }
        if (toks.items[i].ttype >= 256 || !strchr(CHAR_OPS, toks.items[i].ttype)) continue;
        size_t this_precedence = operation_precedence[toks.items[i].ttype];
        if (min_tok_precedence < INIT_TOK_PREC && min_tok_paren_depth < paren_depth) continue;
        if (((toks.items[i].ttype != '=' && (this_precedence > min_tok_precedence)) || 
                    (toks.items[i].ttype == '=' && (this_precedence >= min_tok_precedence))) || 
                paren_depth < min_tok_paren_depth) continue;
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
            } else if (toks.items[i].ttype == JS_TOK_IDENT) {
                *ast = (ASTBranch) {
                    .type = AST_NODE_IDENT,
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
        bool is_binop = strchr(BINOPS, toks.items[min_tok_idx].ttype);
        if ((min_tok_idx < 1 && is_binop) || min_tok_idx >= toks.len) {
            fprintf(stderr, "invalid syntax around operation\n");
            return -1;
        }
        if (!is_binop) { // unary node
            ast->type = AST_NODE_UNARYOP;
            ast->UnaryOp.op = toks.items[min_tok_idx].ttype;
            ast->UnaryOp.val = (ASTBranch*) malloc(sizeof(ASTBranch));
            JSTokens toks_after = toks;
            toks_after.items += min_tok_idx + 1;
            toks_after.len = 1;
            for (size_t depth = toks_after.items[toks_after.len-1].ttype == '(';
                    depth && min_tok_idx + 1 + toks_after.len < toks.len;
                    toks_after.len++) {
                if (toks_after.items[toks_after.len].ttype == '(') depth++;
                else if (toks_after.items[toks_after.len].ttype == ')') depth--;
            }
            if (gen_ast(toks_after, ast->UnaryOp.val) < 0) return -1;
            return 0;
        }
        
        JSTokens toks_before = toks;
        JSTokens toks_after  = toks;
        toks_before.len = min_tok_idx;
        toks_after.items += min_tok_idx + 1;
        toks_after.len -= min_tok_idx + 1;
        if (toks_after.items[toks_after.len - 1].ttype == ')') toks_after.len--;
        if (min_tok_paren_depth) {
            ssize_t open_off = find_xth_tok(min_tok_paren_depth - 1, '(', toks) + 1;
            if (open_off < 0) {
                fprintf(stderr, "uhhh wtf?\n");
                return -1;
            }
            toks_before.items += open_off;
            toks_before.len -= open_off;
            ssize_t closed_off = find_xth_tok(min_tok_paren_depth - 1, ')', toks_after) + 1;
            if (closed_off < 0) {
                fprintf(stderr, "uhhh wtf (2.0)?\n");
                return -1;
            }
            toks_after.len -= closed_off;
        }
        ast->BinOpNode.val1 = (ASTBranch*) malloc(sizeof(ASTBranch));
        ast->BinOpNode.val2 = (ASTBranch*) malloc(sizeof(ASTBranch));
        if (gen_ast(toks_before, ast->BinOpNode.val1) < 0) return -1;
        if (gen_ast(toks_after, ast->BinOpNode.val2) < 0) return -1;
        ast->BinOpNode.op = toks.items[min_tok_idx].ttype;
        ast->type = AST_NODE_BINOP;
    }
    return 0;
}

void print_statement(JSStatement* statement) {
    printf("=================================\n");
    switch (statement->type) {
    case JS_STATEMENT_DEFINE:
        // this should do some proper checks
        printf("Define statement (%s %s)\nSet value to:\n",
                (statement->define_statement.is_const) ? "const" : "let",
                (char*) statement->define_statement.assign_expr->BinOpNode.val1->UnsignedInteger.val);
        dump_ast(statement->define_statement.assign_expr->BinOpNode.val2, 0);
        break;
    default:
        printf("Invalid statement.\n");
    };
    printf("=================================\n");
}

int parse_define_statement(JSTokens* toks, JSStatement* statement) {
    statement->type = JS_STATEMENT_DEFINE;
    statement->define_statement.is_const = toks->items[0].ttype == JS_TOK_CONST;
    JSTokens assign_tokens = *toks;
    assign_tokens.items++, assign_tokens.len--;
    dump_tokens(assign_tokens);
    ASTBranch* assign_ast = (ASTBranch*) malloc(sizeof(ASTBranch));
    if (gen_ast(assign_tokens, assign_ast) < 0) return -1;
    statement->define_statement.assign_expr = assign_ast;
    if (assign_ast->type != AST_NODE_BINOP || assign_ast->BinOpNode.op != '=') {
        fprintf(stderr, "Tried to do let/const define statement with non-assign expression\n");
        free(assign_ast); // TODO: This doesn't actually free everything allocated within
                          // the tree. There should really be a destroy_ast_branch() or something.
        return -1;
    }
    return 0;
}

int js_parse_statement(JSTokens* toks) {
    JSStatement statement = {0};
    switch (toks->items[0].ttype) {
    case JS_TOK_CONST:
    case JS_TOK_LET:
        if (parse_define_statement(toks, &statement) < 0) return -1;
        break;
    default:
        fprintf(stderr, "Invalid statement type.\n");
        return -1;
    }
    print_statement(&statement);
    return 0;
}

int js_parse(JSTokens* toks) {
    size_t start = 0;
    for (size_t i = 0; i < toks->len; i++) {
        if (toks->items[i].ttype != ';') continue;

        JSTokens statement_toks = *toks;
        statement_toks.items += start;
        statement_toks.len = i - start;
        if (js_parse_statement(&statement_toks) < 0) return -1;

        start = i + 1;
        if (toks->items[start].ttype == '\n') start++;
    }
    return 0;
}
