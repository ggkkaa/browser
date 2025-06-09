#include <jsengine.h>

int operation_precendence[] = {
    ['*'] = 1,
    ['/'] = 1,
    ['+'] = 2,
    ['-'] = 2,
};

int gen_ast(JSTokens toks, ASTBranch *ast) {
    (void) toks, (void) ast;
    return 0;
}
