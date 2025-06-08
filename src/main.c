#include <stdio.h>
#include <raylib.h>
#include <fileutils.h>
#include <string.h>
#include <darray.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdint.h>

Vector2 MeasureCodepointEx(Font font, int codepoint, float fontSize, float spacing) {
    size_t index = GetGlyphIndex(font, codepoint);
    float scaleFactor = fontSize/font.baseSize;
    return (Vector2){
        font.glyphs[index].advanceX == 0 ? 
            ((float)font.recs[index].width*scaleFactor + spacing) :
            ((float)font.glyphs[index].advanceX*scaleFactor + spacing),
        fontSize
    };
}
#include <atom.h>
#include <atom_set.h>
Atom* atom_new(const char* data, size_t n) {
    Atom* atom = malloc(sizeof(*atom) + n + 1);
    assert(atom && "Just buy more RAM");
    atom->len = n;
    memcpy(atom->data, data, n);
    atom->data[n] = '\0';
    return atom;
}
Atom* atom_new_cstr(const char* data) {
    return atom_new(data, strlen(data));
}
#define W_RATIO 16
#define H_RATIO 9
#define SCALE 100
#define WIDTH  W_RATIO*SCALE
#define HEIGHT H_RATIO*SCALE
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
enum {
    HTMLERR_TODO=1,
    HTMLERR_EOF,
    HTMLERR_INVALID_TAG,
    HTMLERR_INVALID_ATTRIBUTE,
    HTMLERR_COUNT,
};
static_assert(HTMLERR_COUNT == 5, "Update htmlerr_strtab");
const char* htmlerr_strtab[] = {
    [HTMLERR_TODO] = "Unimplemented",
    [HTMLERR_EOF]  = "End of File",
    [HTMLERR_INVALID_TAG] = "Invalid tag format",
    [HTMLERR_INVALID_ATTRIBUTE]  = "Invalid attribute format",
};
const char* htmlerr_str(int err) {
    if(err >= 0) return "OK";
    err = -err;
    if(err >= HTMLERR_COUNT) return "Unknown error";
    return htmlerr_strtab[err];
}

int parse_attribute(const char* content, HTMLAttribute* att, const char** end) {
    att->key = (char*)content;
    while (isalnum(*content) || *content == '_' || *content == '-')
        content++;
    att->key_len = content - att->key;
    while(isspace(*content)) content++;
    if (*content != '=') {
        // it has no value
        *end = content;
        return 0;
    }
    content++;
    while (isspace(*content)) content++;
    if (*content != '"') return -HTMLERR_INVALID_ATTRIBUTE;
    content++;
    att->value = (char*)content;
    while(*content && *content != '"') content++;
    if(*content != '"') return -HTMLERR_EOF;
    att->value_len = content - att->value;
    content++;
    *end = content;
    return 0;
}

#define STRINGIFY0(x) # x
#define STRINGIFY1(x) STRINGIFY0(x)
#define todof(...) (fprintf(stderr, "TODO " __FILE__ ":" STRINGIFY1(__LINE__) ":" __VA_ARGS__), abort())
int html_parse_next_tag(AtomTable* atom_table, const char* content, HTMLTag* tag, char** end) {
    if(*content == '<') {
        content++;
        const char* name = content;
        while(isalnum(*content)) content++;
        tag->name = atom_table_get(atom_table, name, content-name);
        if(!tag->name) {
            tag->name = atom_new(name, content - name);
            assert(atom_table_insert(atom_table, tag->name) && "Just buy more RAM");
        }
        while (*content && *content != '>' && *content != '/') {
            while(isspace(*content)) content++;
            int e;
            HTMLAttribute *att = (HTMLAttribute*) malloc(sizeof(HTMLAttribute));
            assert(att && "Just buy more RAM");
            memset(att, 0, sizeof(*att));
            if ((e=parse_attribute(content, att, &content)) < 0) return e;
            da_push(&tag->attributes, att);
        }
        if (*content == '/') {
            if (content[1] != '>') return -HTMLERR_INVALID_TAG;
            content += 2;
            *end = (char*) content;
            tag->self_closing = true;
            return 0;
        }
        tag->self_closing = false;
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
        if(tag->name->len == 5 && strncmp(tag->name->data, "style", 5) == 0) return;
        printf("%*s<%.*s>\n", (int)indent, "", (int)tag->name->len, tag->name->data);
        for(size_t i = 0; i < tag->children.len; ++i) {
            dump_html_tag(tag->children.items[i], indent + 4);
        }
        printf("%*s</%.*s>\n", (int)indent, "", (int)tag->name->len, tag->name->data);
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
void compute_box_html_tag(HTMLTag* tag, Font font, float fontSize, float textFontSize, float spacing, size_t* cursor_x, size_t* cursor_y) {
    size_t new_x = tag->x = *cursor_x;
    size_t new_y = tag->y = *cursor_y;
    size_t max_x = tag->x;
    size_t max_y = tag->y;
    if(tag->name) {
        for(size_t i = 0; i < tag->children.len; ++i) {
            HTMLTag* child = tag->children.items[i];
            // if(child->display == CSSDISPLAY_BLOCK && tag->display == CSSDISPLAY_INLINE) todof("We do not support block inside inline atm: (parent=%.*s, child=%.*s)\n", (int)tag->name->len, tag->name, (int)child->name->len, child->name);
            if(child->display == CSSDISPLAY_BLOCK) {
                new_x = tag->x;
                new_y = max_y;
            }
            float childFontSize = fontSize;
            if(strncmp(tag->name->data, "h1", 2) == 0) childFontSize = fontSize * 1.0;
            else if(strncmp(tag->name->data, "h2", 2) == 0) childFontSize = fontSize * 0.83;
            else if(strncmp(tag->name->data, "h3", 2) == 0) childFontSize = fontSize * 0.75;
            else if(strncmp(tag->name->data, "p", 1) == 0 || strncmp(tag->name->data, "strong", 6) == 0) childFontSize = fontSize * 0.66;
            else if(strncmp(tag->name->data, "li", 2) == 0) childFontSize = fontSize * 0.5;
            compute_box_html_tag(child, font, fontSize, childFontSize, spacing, &new_x, &new_y);
            if(child->x + child->width > max_x) max_x = child->x + child->width;
            if(child->y + child->height > max_y) max_y = child->y + child->height;
        }
    } else {
        float x = new_x;
        float y = new_y;
        // TODO: unhardcode this
        // Take as parameter
        float max_width = GetScreenWidth();
        for(size_t i = 0; i < tag->str_content_len; ++i) {
            char c = tag->str_content[i];
            if(isspace(c)) {
                while(i+1 < tag->str_content_len && isspace(tag->str_content[i+1])) i++;
                c = ' ';
            } else if (!isgraph(c)) c = '?';
            Vector2 size = MeasureCodepointEx(font, c, textFontSize, spacing);
            if(x + size.x > max_width) {
                // TODO: unhardcode this?
                x = 0;// tag->x;
                y += textFontSize;
            }
            x += size.x;
            if(x + size.x > max_x) max_x = x + size.x;
            if(y + size.y > max_y) max_y = y + size.y;
        }
        new_x = x;
        new_y = y;
    }
    tag->width = max_x - tag->x;
    tag->height = max_y - tag->y;
    static_assert(CSSDISPLAY_COUNT == 3, "Update compute_box_html_tag");
    switch(tag->display) {
    case CSSDISPLAY_BLOCK:
        *cursor_y = max_y;
        *cursor_x = tag->x;
        break;
    case CSSDISPLAY_INLINE:
        *cursor_x = new_x;
        *cursor_y = new_y;
        break;
    case CSSDISPLAY_INLINE_BLOCK:
        *cursor_x = max_x;
        *cursor_y = tag->y;
        break;
    default:
        todof("handle display: %d", tag->display);
    }
}

static size_t color_n = 0;
void render_box_html_tag(HTMLTag* tag, float scroll_y) {
    static Color colors[] = {
        GRAY,     
        GOLD,     
        PINK,     
        SKYBLUE,  
        RED,      
        MAROON,   
        GREEN,    
        ORANGE,   
        LIME,     
        DARKGREEN,
        BLUE,     
        DARKBLUE, 
        PURPLE,   
        VIOLET,   
        DARKPURPLE,
        BEIGE,    
        BROWN,    
        DARKBROWN,
        WHITE,    
        BLANK,    
        MAGENTA,  
        RAYWHITE, 
    };
    DrawRectangle(tag->x, ((float)tag->y) + scroll_y, tag->width, tag->height, colors[color_n++]);
    for(size_t i = 0; i < tag->children.len; ++i) {
        render_box_html_tag(tag->children.items[i], scroll_y);
    }
}
void render_html_tag(HTMLTag* tag, Font font, float fontSize, float textFontSize, float spacing, float scroll_y) {
    if(tag->name) {
        if(tag->name->len == 5 && strncmp(tag->name->data, "style", 5) == 0) return;
        if(tag->name->len == 5 && strncmp(tag->name->data, "title", 5) == 0) return;
        for(size_t i = 0; i < tag->children.len; ++i) {
            float childFontSize = fontSize;
            if(strncmp(tag->name->data, "h1", 2) == 0) childFontSize = fontSize * 1.0;
            else if(strncmp(tag->name->data, "h2", 2) == 0) childFontSize = fontSize * 0.83;
            else if(strncmp(tag->name->data, "h3", 2) == 0) childFontSize = fontSize * 0.75;
            else if(strncmp(tag->name->data, "p", 1) == 0 || strncmp(tag->name->data, "strong", 6) == 0) childFontSize = fontSize * 0.66;
            else if(strncmp(tag->name->data, "li", 2) == 0) childFontSize = fontSize * 0.5;
            render_html_tag(tag->children.items[i], font, fontSize, childFontSize, spacing, scroll_y);
        }
    } else {
        float x = tag->x, y = tag->y; 
        for(size_t i = 0; i < tag->str_content_len; ++i) {
            char c = tag->str_content[i];
            if(isspace(c)) {
                while(i+1 < tag->str_content_len && isspace(tag->str_content[i+1])) i++;
                c = ' ';
            } else if (!isgraph(c)) c = '?';

            Vector2 size = MeasureCodepointEx(font, c, textFontSize, spacing);
            if(x + size.x > tag->x + tag->width) {
                //TODO: Unhardcode this?
                x = 0;// tag->x;
                y += textFontSize;
            }
            DrawTextCodepoint(font, c, (Vector2){x, y + scroll_y}, textFontSize, BLACK);
            x += size.x;
        }
    }
}

HTMLTag* find_child_html_tag(HTMLTag* tag, const char* name) {
    if(!tag) return NULL;
    size_t name_len = strlen(name);
    for(size_t i = 0; i < tag->children.len; ++i) {
        HTMLTag* child = tag->children.items[i];
        if(child->name->len == name_len && memcmp(child->name->data, name, name_len) == 0) return child;
    }
    return NULL;
}
// FIXME: I know strcasecmp exists and we *can* use that on 
// Unix systems with a flag but for now this is fine
static int strncmp_ci(const char *restrict s1, const char *restrict s2, size_t max) {
   while(max && *s1 && *s2 && (tolower(*s1) == tolower(*s2))) {
        s1++;
        s2++;
        max--;
   }
   if(max != 0) return ((unsigned char)*s1) - ((unsigned char)*s2);
   return 0;
}
char* shift_args(int* argc, char*** argv) {
    return (*argc) <= 0 ? NULL : ((*argc)--, *((*argv)++));
}
void help(FILE* sink, const char* exe) {
    fprintf(sink, "%s <input path>\n", exe);
}
int main(int argc, char** argv) {
    const char* exe = shift_args(&argc, &argv);
    const char* example_path = NULL;

    while(argc) {
        const char* arg = shift_args(&argc, &argv);
        if (strcmp(arg, "--help") == 0) {
            help(stdout, exe);
            return 0;
        }
        else if(!example_path) example_path = arg;
        else {
            fprintf(stderr, "ERROR Unexpected argument: `%s`\n", arg);
            help(stderr, exe);
            return 1;
        }
    }
    if(!example_path) {
        fprintf(stderr, "ERROR Missing input path!\n");
        help(stderr, exe);
        return 1;
    }

    size_t content_size;
    char* content_data = (char*)read_entire_file(example_path, &content_size);
    if(!content_data) return 1;
    char* content = content_data;
    bool quirks_mode = true;
    if(strncmp_ci(content, "<!DOCTYPE", 9) == 0) {
        content += 9;
        while(isspace(*content)) content++;
        if(strncmp_ci(content, "html", 4) != 0) todof("Unsupported DOCTYPE: `%.*s`", 4, content);
        content += 4;
        while(*content && *content != '>') content++;
        if(*content != '>') {
            todof("Unterminated DOCTYPE html");
        }
        content++;
        quirks_mode = false;
    }
    (void) quirks_mode;
    HTMLTag root = { 0 };
    AtomTable atom_table = { 0 };
    AtomSet block_elements = { 0 };
    const char* block_tags[] = {
        "address",
        "article",
        "aside",
        "blockquote",
        "br",
        "dd",
        "dl",
        "dt",
        "fieldset",
        "figcaption",
        "figure",
        "footer",
        "form",
        "h1",
        "h2",
        "h3",
        "h4",
        "h5",
        "h6",
        "header",
        "hgroup",
        "hr",
        "li",
        "main",
        "nav",
        "ol",
        "p",
        "pre",
        "section",
        "table",
        "ul",
        "details",
        "dialog",
        "summary",
        "menu",
        "tfoot",
        "thead",
        "div",
        "body",
        "html",
    };
    for(size_t i = 0; i < sizeof(block_tags)/sizeof(*block_tags); ++i) {
        Atom* atom = atom_new_cstr(block_tags[i]);
        atom_table_insert(&atom_table, atom);
        atom_set_insert(&block_elements, atom);
    }
    root.name = atom_new("\\root", 5);
    assert(atom_table_insert(&atom_table, root.name) && "Just buy more RAM");
    root.display = CSSDISPLAY_BLOCK;
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
        if(content[0] == '<' && content[1] == '!' && content[2] == '-' && content[3] == '-') {
            content += 4;
            while(*content && (content[0] != '-' || content[1] != '-' || content[2] != '>')) content++;
            if(*content) content += 3;
            continue;
        }
        HTMLTag* tag = malloc(sizeof(*tag));
        assert(tag && "Just buy more RAM");
        memset(tag, 0, sizeof(*tag));
        int e = html_parse_next_tag(&atom_table, content, tag, &content);
        if(tag->name && atom_set_get(&block_elements, tag->name)) {
            tag->display = CSSDISPLAY_BLOCK;
        }
        if(e == -HTMLERR_EOF) break;
        tag->parent = node; 
        da_push(&node->children, tag);
        if (!tag->self_closing && tag->name) node = tag;
        if(e != 0) {
            fprintf(stderr, "Failed to parse tag: %s\n", htmlerr_str(e));
            return 1;
        }
    }
    if(node != &root) {
        HTMLTag* ct = node;
        fprintf(stderr, "WARN: Some unclosed tags:\n");
        while(ct != &root) {
            if(!ct->name) {
                fprintf(stderr, "- <unnamed>\n");
                continue;
            }
            fprintf(stderr, "- %.*s\n", (int)ct->name->len, ct->name->data);
            ct = ct->parent;
        }
    }
    dump_html_tag(node, 0);

    HTMLTag* html = find_child_html_tag(&root, "html");
    HTMLTag* head = find_child_html_tag(html, "head");
    HTMLTag* body = find_child_html_tag(html, "body");
    HTMLTag* title = find_child_html_tag(head, "title");
    const char* window_title = "Bikeshed";
    if(title && title->children.len && !title->children.items[0]->name) {
        HTMLTag* title_str = title->children.items[0];
        window_title = TextFormat("Bikeshed - %.*s", (int)title_str->str_content_len, title_str->str_content);
    }
    InitWindow(WIDTH, HEIGHT, window_title);
    Font font = LoadFont("fonts/iosevka/iosevka-bold.ttf");
    float fontSize = 32.0f;
    float spacing = fontSize*0.1f;
    if (font.texture.id == 0) {
        font = GetFontDefault();
        fontSize = 24.0f;
        spacing = fontSize*0.1f;
    }
    SetTargetFPS(60);
    float scroll_y = 0;
    bool show_boxes = false;
    while(!WindowShouldClose()) {
        scroll_y += GetMouseWheelMove()*16.0;
        if(IsKeyReleased(KEY_F4)) show_boxes = !show_boxes;
        BeginDrawing();
        ClearBackground(RAYWHITE);
        size_t x = 0, y = 0;
        color_n = 0;
        compute_box_html_tag(body, font, fontSize, fontSize, spacing, &x, &y);
        if(show_boxes) render_box_html_tag(body, scroll_y);
        render_html_tag(body, font, fontSize, fontSize, spacing, scroll_y);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
