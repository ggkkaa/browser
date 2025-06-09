#pragma once
#include <stdlib.h>

enum {
    HTMLERR_TODO=1,
    HTMLERR_EOF,
    HTMLERR_INVALID_TAG,
    HTMLERR_INVALID_ATTRIBUTE,
    HTMLERR_INVALID_JS, // this is very vague only because the JS engine itself will
                        // write more detailed info separately to stderr
    HTMLERR_COUNT,
};

#define STRINGIFY0(x) # x
#define STRINGIFY1(x) STRINGIFY0(x)
#define todof(...) (fprintf(stderr, "TODO " __FILE__ ":" STRINGIFY1(__LINE__) ":" __VA_ARGS__), abort())
