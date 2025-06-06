#include "cssparser.h"
#include "darray.h"
#include <string.h>
#include <ctype.h>
#include <assert.h>

const char* csserr_strtab[] = {
    [CSSERR_TODO] = "Unimplemented",
    [CSSERR_EOF]  = "End of File",
    [CSSERR_ATTR] = "Couldn't parse attribute"
};

const char* csserr_str(int err) {
    if(err >= 0) return "OK";
    err = -err;
    if(err >= CSSERR_COUNT) return "Unknown error";
    return csserr_strtab[err];
}

#define CSS_MALLOC malloc
#define CSS_FREE(ptr) (free(ptr))

//TODO: trim comments `/* css comment */`
char* trim_left(char* cur){
    char *out_cur = cur;
    while(isspace(*out_cur) != 0) out_cur++;
    return out_cur;
}

int parse_css_attr(const char* content, char** content_end, CSSAttr* outAttr){
    char* cur = (char*)content;
    cur = trim_left(cur);
    if(*cur == '}') return 0;
    if(*(cur+1) == 0) return -CSSERR_EOF;

    char* name_content = cur;
    size_t name_len = 0;

    cur = strchr(cur,':');
    if(*(cur+1) == 0) return -CSSERR_EOF;
    name_len = cur-name_content;
    cur++;

    cur = trim_left(cur);
    char* value_content = cur;
    size_t value_len = 0;

    while(*cur != ';') cur++;
    if(*(cur+1) == 0) return -CSSERR_EOF;
    value_len = cur - value_content;
    cur++;

    outAttr->name_content = name_content;
    outAttr->name_len = name_len;
    outAttr->value_content = value_content;
    outAttr->value_len = value_len;
    *content_end = cur;
    return 1;
}

int parse_css_node(const char* content, char** content_end, CSSNode* outNode){
    char *cur = (char*)content;
    cur = trim_left(cur);
    if(*(cur+1) == 0) return -CSSERR_EOF;

    char* name_content = cur;
    size_t name_len = 0;

    cur = strchr(cur,'{');
    if(*(cur+1) == 0) return -CSSERR_EOF;
    name_len = cur-name_content;
    cur++;

    CSSAttrs attrs = {0};
    CSSAttr attr = {0};

    int result = 1;
    while(result > 0){
        result = parse_css_attr(cur,&cur,&attr);
        if(result < 0) return result;
        da_push(&attrs, attr);
    }

    cur = strchr(cur,'}')+1;
    
    memset(outNode,0, sizeof(CSSNode));
    outNode->name_content = name_content;
    outNode->name_len = name_len;
    outNode->attrs = attrs;
    *content_end = cur;
    return 0;
}

int parse_css_file(const char* content, CSSNodes* outNodes){
    char* cur = (char*)content;
    CSSNode node = {0};

    int result = 0;
    while(*cur != '\0'){
        result = parse_css_node(cur, &cur, &node);
        if(result < 0) return result;
        da_push(outNodes, node);
    }

    return 0;
}

int parse_css_buffer(const char* content, size_t size, CSSNodes* outNodes){
    char* cur = (char*)content;
    CSSNode node = {0};

    int result = 0;
    while(cur < content+size){
        if(*cur == '\0') return -CSSERR_EOF;
        result = parse_css_node(cur, &cur, &node);
        if(result < 0) return result;
        da_push(outNodes, node);
    }

    return 0;
}