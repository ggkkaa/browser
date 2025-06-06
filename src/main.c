#include <stdio.h>
#include <raylib.h>
#include <fileutils.h>
#include <string.h>
#include <darray.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

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
        // For now we skip everything until the >
        while(*content && *content != '>') content++;
        // if(*content != '>') todof("parse attributes");
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

//TODO: Handle html comments `<!-- html comment!-->` because it cannot load title in motherfuckingwebsite.html
void dump_html_tag(HTMLTag* tag, size_t indent) {
    if(tag->name) {
        if(tag->name_len == 5 && strncmp(tag->name, "style", 5) == 0) return;
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
void render_html_tag(HTMLTag* tag, float fontSize, float* rx, float* ry) {
    if(tag->name) {
        if(tag->name_len == 5 && strncmp(tag->name, "style", 5) == 0) return;
        if(tag->name_len == 5 && strncmp(tag->name, "title", 5) == 0) return;
        for(size_t i = 0; i < tag->children.len; ++i) {
            float childFontSize = fontSize;
            if(strncmp(tag->name, "h1", 2) == 0) childFontSize = 24.0;
            else if(strncmp(tag->name, "h2", 2) == 0) childFontSize = 20.0;
            else if(strncmp(tag->name, "h3", 2) == 0) childFontSize = 18.0;
            else if(strncmp(tag->name, "p", 1) == 0) childFontSize = 12.0;
            else if(strncmp(tag->name, "li", 2) == 0) childFontSize = 10.0;
            else {
                childFontSize = 11.0;
            }
            render_html_tag(tag->children.items[i], childFontSize, rx, ry);
        }
    } else {
        Font font = GetFontDefault();
        float x = *rx, y = *ry; 
        size_t width = GetScreenWidth();
        for(size_t i = 0; i < tag->str_content_len; ++i) {
            char c = tag->str_content[i];
            if(!isgraph(c) && c != ' ') c = '?';
            if(x + fontSize > width) {
                x = *rx;
                y += fontSize;
            }
            size_t index = GetGlyphIndex(font, c);
            Vector2 pos = {
                x, y
            };
            DrawTextCodepoint(font, c, pos, fontSize, BLACK);
            float scaleFactor = fontSize/font.baseSize;
            int defaultFontSize = 10;   // Default Font chars height in pixel
            if (fontSize < defaultFontSize) fontSize = defaultFontSize;
            int spacing = fontSize/defaultFontSize;
            if (font.glyphs[index].advanceX == 0) x += ((float)font.recs[index].width*scaleFactor + spacing);
            else x += ((float)font.glyphs[index].advanceX*scaleFactor + spacing);
        }
        *ry = y + fontSize;
    }
}
HTMLTag* find_child_html_tag(HTMLTag* tag, const char* name) {
    if(!tag) return NULL;
    size_t name_len = strlen(name);
    for(size_t i = 0; i < tag->children.len; ++i) {
        HTMLTag* child = tag->children.items[i];
        if(child->name_len == name_len && memcmp(child->name, name, name_len) == 0) return child;
    }
    return NULL;
}

int main(int argc, char** argv) {
    const char* example_path;

    if(argc > 1) {
        example_path = (char*)argv[1];
    }
    else{
        fprintf(stderr, "Provide filepath\n");
        return 1;
    }
    
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

    HTMLTag* html = find_child_html_tag(&root, "html");
    HTMLTag* head = find_child_html_tag(html, "head");
    HTMLTag* title = find_child_html_tag(head, "title");
    const char* window_title = "Bikeshed";
    if(title && title->children.len && !title->children.items[0]->name) {
        HTMLTag* title_str = title->children.items[0];
        window_title = TextFormat("Bikeshed - %.*s", (int)title_str->str_content_len, title_str->str_content);
    }
    InitWindow(WIDTH, HEIGHT, window_title);
    SetTargetFPS(60);
    float scroll_y = 0;
    while(!WindowShouldClose()) {
        scroll_y += GetMouseWheelMove()*6.0;
        BeginDrawing();
        ClearBackground(RAYWHITE);
        float x = 0, y = scroll_y;
        render_html_tag(node, 24.0, &x, &y);
        EndDrawing();
    }
    CloseWindow();
}
