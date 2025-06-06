#include "cssparser.h"
#include "darray.h"
#include <string.h>
#include <ctype.h>
#include <assert.h>

const char* csserr_strtab[] = {
    [CSSERR_TODO] = "Unimplemented",
    [CSSERR_EOF]  = "End of File",
    [CSSERR_MISSING_SEPERATOR] = "Missing :",
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

char* trim_left(char* cur, const char* content_end){
    char *out_cur = cur;
    while(isspace(*out_cur) != 0 && out_cur < content_end) out_cur++;

    return out_cur;
}

int parse_css_attr(char** cur, const char* content_end, CSSAttr* outAttr){
    *cur = trim_left(*cur, content_end);
    if(**cur == '}') return 0;
    if(*cur + 1 >= content_end) return -CSSERR_EOF;

    char* name_content = *cur;
    size_t name_len = 0;

    *cur = strchr(*cur,':');
    if(*cur + 1 >= content_end) return -CSSERR_MISSING_SEPERATOR;
    name_len = *cur-name_content;

    *cur += 1;
    *cur = trim_left(*cur, content_end);
    char* value_content = *cur;
    size_t value_len = 0;

    while(**cur != '\r' && **cur != '\n' && *cur < content_end) *cur += 1;
    if(*cur + 1 >= content_end) return -CSSERR_EOF;

    value_len = *cur - value_content;

    outAttr->name_content = name_content;
    outAttr->name_len = name_len;
    outAttr->value_content = value_content;
    outAttr->value_len = value_len;
    return 1;
}

int parse_css_node(char** cur, const char* content_end, CSSNode* outNode){
    *cur = trim_left(*cur, content_end);
    if(*cur + 1 >= content_end) return 0;

    char* name_content = *cur;
    size_t name_len = 0;

    *cur = strchr(*cur,'{');
    if(*cur+1 >= content_end) return -CSSERR_EOF;
    name_len = *cur-name_content;
    *cur += 1;

    CSSAttrs attrs = {0};
    CSSAttr attr = {0};

    int result = 1;
    while(result > 0){
        result = parse_css_attr(cur,content_end,&attr);
        if(result < 0) return result;
        da_push(&attrs, attr);
    }

    *cur = strchr(*cur,'}')+1;

    memset(outNode,0, sizeof(CSSNode));
    outNode->name_content = name_content;
    outNode->name_len = name_len;
    outNode->attrs = attrs;
    return 0;
}

int parse_css_file(const char* content, CSSNodes* outNodes){
    size_t content_len = strlen(content);
    char* cur = (char*)content;
    CSSNode node = {0};

    int result = 0;
    while(cur < content+content_len){
        result = parse_css_node(&cur, content+content_len, &node);
        if(result < 0) return result;
        da_push(outNodes, node);
    }

    return 0;
}