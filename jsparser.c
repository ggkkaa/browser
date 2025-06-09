#include <jsengine.h>

typedef enum {
    AST_NODE_BINOP;
    AST_NODE_UNSIGNED_INT;
} ASTNodeType;

typedef struct ASTBranch ASTBranch;
struct ASTBranch {
    ASTNodeType type;
    union {
        struct {
            ASTBranch val1;
            int op; // ascii for the symbol
            ASTBranch val2;
        } BinOpNode;
        struct {
            uint64_t val;
        } UnsignedInteger;
    };
};
