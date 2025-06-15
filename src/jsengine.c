#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <jsengine.h>
#include <assert.h>
#include <todo.h>
#include <stddef.h>
#include <darray.h>

int run_js(char* content) {
    JSTokens toks = {0};
    if (tokenise_js(&toks, content) < 0) {
        fprintf(stderr, "Failed to tokenise JS\n");
        return -1; //-HTMLERR_INVALID_JS;
    }
    printf("Tokenising complete. Token dump:\n");
    dump_tokens(toks);
    ASTBranch ast = {0};
    if (gen_ast(toks, &ast) < 0) {
        fprintf(stderr, "Failed to parse JS and generate an AST\n");
        return -1;
    }
    printf("Done parsing. AST dump:\n");
    dump_ast(&ast, 0);
    return 0;
}
