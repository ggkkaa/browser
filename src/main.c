#include <stdio.h>
#include <raylib.h>
#include <fileutils.h>
#include <string.h>
#include <darray.h>
#include <assert.h>
#include <ctype.h>

#define W_RATIO 16
#define H_RATIO 9
#define SCALE 100
#define WIDTH  W_RATIO*SCALE
#define HEIGHT H_RATIO*SCALE
typedef struct HTMLTag HTMLTag;
typedef struct {
    HTMLTag** items;
    size_t len, cap;
} HTMLTags;
struct HTMLTag {
    HTMLTag* parent;
    const char* name;
    size_t name_len;
    HTMLTags children;
    const char* str_content;
    size_t str_content_len;
};
enum {
    HTMLERR_TODO=1,
    HTMLERR_EOF,
    HTMLERR_COUNT
};
static_assert(HTMLERR_COUNT == 3, "Update htmlerr_strtab");
const char* htmlerr_strtab[] = {
    [HTMLERR_TODO] = "Unimplemented",
    [HTMLERR_EOF]  = "End of File",
};
const char* htmlerr_str(int err) {
    if(err >= 0) return "OK";
    err = -err;
    if(err >= HTMLERR_COUNT) return "Unknown error";
    return htmlerr_strtab[err];
}

#define STRINGIFY0(x) # x
#define STRINGIFY1(x) STRINGIFY0(x)
#define todof(...) (fprintf(stderr, "TODO " __FILE__ ":" STRINGIFY1(__LINE__) ":" __VA_ARGS__), abort())
int html_parse_next_tag(const char* content, HTMLTag* tag, char** end) {
    if(*content == '<') {
        content++;
        tag->name = content;
        while(isalnum(*content)) content++;
        tag->name_len = content - tag->name;
        if(*content != '>') todof("parse attributes");
        content++;
        *end = (char*)content;
        return 0;
    }
    if(*content == '\0') return -HTMLERR_EOF;
    tag->str_content = content;
    while(*content != '<' && *content) content++;
    tag->str_content_len = content - tag->str_content;
    *end = (char*)content;
    return 0;
}
void dump_html_tag(HTMLTag* tag, size_t indent) {
    if(tag->name) {
        printf("%*s<%.*s>\n", (int)indent, "", (int)tag->name_len, tag->name);
        for(size_t i = 0; i < tag->children.len; ++i) {
            dump_html_tag(tag->children.items[i], indent + 4);
        }
        printf("%*s</%.*s>\n", (int)indent, "", (int)tag->name_len, tag->name);
    } else {
        printf("%*s", (int)indent, "");
        for(size_t i = 0; i < tag->str_content_len; ++i) {
            char c = tag->str_content[i];
            if(isgraph(c) || c == ' ') printf("%c", c);
            else printf("\\x%02X", c);
        }
        printf("\n");
    }
}
int main(void) {
    // TODO: Unhardcode this sheizung
    const char* example_path = "examples/barebones.html";
    size_t content_size;
    char* content_data = (char*)read_entire_file(example_path, &content_size);
    char* content = content_data;
    bool quirks_mode = true;
    if(!memcmp(content, "<!DOCTYPE html>", 15)) {
        content += 15;
        quirks_mode = false;
    }
    (void) quirks_mode;
    HTMLTag root = { 0 };
    root.name = "\\root";
    root.name_len = strlen(root.name);
    HTMLTag* node = &root;
    for(;;) {
        while(isspace(*content)) content++;
        if(*content == '\0') break;
        if(content[0] == '<' && content[1] == '/') {
            while(*content != '>' && *content) content++;
            content++;
            node = node->parent;
            continue;
        }
        HTMLTag* tag = malloc(sizeof(*tag));
        assert(tag && "Just buy more RAM");
        memset(tag, 0, sizeof(*tag));
        da_push(&node->children, tag);
        tag->parent = node; 
        int e = html_parse_next_tag(content, tag, &content);
        if(e == -HTMLERR_EOF) break;
        if(tag->name) node = tag;
        if(e != 0) {
            fprintf(stderr, "Failed to parse tag: %s\n", htmlerr_str(e));
            return 1;
        }
    }
    if(node != &root) {
        fprintf(stderr, "WARN: Some unclosed tags.\n");
    }
    dump_html_tag(node, 0);
    InitWindow(WIDTH, HEIGHT, "Bikeshed");
    while(!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText(content, 0, 0, 0, BLACK);
        EndDrawing();
    }
    CloseWindow();
}
