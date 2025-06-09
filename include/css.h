#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

enum {
    CSSERR_EOF=1,
    CSSERR_INVALID_TAG_NAME,
    CSSERR_INVALID_ATTRIBUTE_SYNTAX,
    CSSERR_INVALID_ARG_SYNTAX,

    CSSERR_COUNT
};
enum {
    // #foo
    CSSTAG_ID,
    // .foo
    CSSTAG_CLASS,
    // foo
    CSSTAG_TAG,
};
typedef struct AtomTable AtomTable;
typedef struct Atom Atom;
typedef uint32_t CSSTagKind;
typedef struct {
    Atom* name;
    CSSTagKind kind;
} CSSTag;
typedef struct {
    char* value;
    size_t value_len;
} CSSArg;
typedef struct {
    Atom* name;
    struct {
        CSSArg* items;
        size_t len, cap;
    } args;
} CSSAttribute;
typedef struct {
    CSSAttribute* items;
    size_t len, cap;
} CSSAttributes;
typedef struct {
    CSSTag* items;
    size_t len, cap;
    // TODO: CSSTag inline_buffer[1];
    CSSAttributes attribute;
} CSSPattern;

// Skip spaces and comments
const char* css_skip(const char* content, const char* content_end);
const char* csserr_str(int err);
int css_parse_tag(AtomTable* atom_table, const char* content, const char* content_end, char** end, CSSTag* tag);
int css_parse_attribute(AtomTable* atom_table, const char* content, const char* content_end, char** end, CSSAttribute* att);
