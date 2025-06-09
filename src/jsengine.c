#include <stdio.h>
#include <jsengine.h>
#include <miscutils.h>

int run_js(char* content) {
    JSTokens toks = {0};
    if (tokenise_js(&toks, content) < 0) {
        fprintf(stderr, "Failed to tokenise JS\n");
        return -HTMLERR_INVALID_JS;
    }
    dump_tokens(toks);
    ASTBranch ast = {0};
    if (gen_ast(toks, &ast) < 0) {
        fprintf(stderr, "Failed to parse JS and generate an AST\n");
        return -HTMLERR_INVALID_JS;
    }
    return 0;
}
