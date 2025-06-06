#pragma once

#include "darray.h"
#include <assert.h>

typedef struct{
    const char* name_content;
    size_t name_len;
    char* value_content;
    size_t value_len;
} CSSAttr;

typedef struct{
    CSSAttr* items;
    size_t len,cap;
} CSSAttrs;

typedef struct {
    const char* name_content;
    size_t name_len;
    CSSAttrs attrs;
} CSSNode;

typedef struct {
    CSSNode* items;
    size_t len,cap;
} CSSNodes;

int parse_css_file(const char* content, CSSNodes* outNodes);

enum {
    CSSERR_TODO=1,
    CSSERR_EOF,
    CSSERR_ATTR,
    CSSERR_COUNT
};
static_assert(CSSERR_COUNT == 4, "Update csserr_strtab");

extern const char* csserr_strtab[CSSERR_COUNT];
const char* csserr_str(int err);