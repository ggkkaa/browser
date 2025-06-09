// Pedal: the ECMAScript bytecode compiler + interpreter for Bikeshed
#include <stdio.h>
#include <miscutils.h>
#include <stddef.h>
#include <darray.h>

typedef struct JSToken JSToken;
typedef struct {
    JSToken** items;
    size_t len, cap;
} JSTokens;

typedef enum {
    JSInteger, JSPlus, JSMinus, JSMul, JSDiv,
} JSTokType;

struct JSToken {
    JSTokType ttype;
    size_t val; // what this should be casted to depends on `ttype`
};

int run_js(char* content) {
    (void) content;
    todof("JS engine\n");
    return 0;
}
