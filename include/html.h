#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
enum {
    HTMLERR_TODO=1,
    HTMLERR_EOF,
    HTMLERR_INVALID_TAG,
    HTMLERR_INVALID_ATTRIBUTE,
    HTMLERR_COUNT,
};
typedef struct Atom Atom;
typedef struct HTMLAttribute HTMLAttribute;
typedef struct {
    HTMLAttribute** items;
    size_t len, cap;
} HTMLAttributes;
struct HTMLAttribute {
    char* key;
    size_t key_len;
    char* value; // value is NULL if it has no value
    size_t value_len;
};
typedef struct HTMLTag HTMLTag;
typedef struct {
    HTMLTag** items;
    size_t len, cap;
} HTMLTags;
// TODO: refactor this out to CSS
// and some sort of CSSStyle
enum {
    CSSDISPLAY_INLINE,
    CSSDISPLAY_INLINE_BLOCK,
    CSSDISPLAY_BLOCK,

    CSSDISPLAY_COUNT
};
typedef uint32_t CSSDisplay;
struct HTMLTag {
    HTMLTag* parent;
    Atom* name;
    HTMLTags children;
    const char* str_content;
    size_t str_content_len;
    HTMLAttributes attributes;
    CSSDisplay display;
    bool self_closing;
    // Box of the tag 
    size_t x, y;
    size_t width, height;
};

const char* htmlerr_str(int err);
