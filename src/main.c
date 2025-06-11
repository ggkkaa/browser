#include <stdio.h>
#include <jsengine.h>
#include <todo.h>
#include <raylib.h>
#include <fileutils.h>
#include <string.h>
#include <darray.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <atom.h>
#include <atom_set.h>
#include "html.h"
#include <css_pattern_map.h>

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
#define W_RATIO 16
#define H_RATIO 9
#define SCALE 100
#define WIDTH  W_RATIO*SCALE
#define HEIGHT H_RATIO*SCALE
#define ARRAY_LEN(a) (sizeof(a)/sizeof(*a))

void compute_box_html_tag(HTMLTag* tag, Font font, float fontSize, float textFontSize, float spacing, size_t* cursor_x, size_t* cursor_y) {
    size_t new_x = tag->x = *cursor_x;
    size_t new_y = tag->y = *cursor_y;
    size_t max_x = tag->x;
    size_t max_y = tag->y;
    if(tag->name) {
        if(strcmp(tag->name->data, "script") == 0) return;
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
        if(strcmp(tag->name->data, "script") == 0) return;
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
void match_css_patterns(HTMLTag* tag, CSSPatternMap* tags) {
    if(tags->len == 0) return;
    CSSPatterns* patterns = css_pattern_map_get(tags, tag->name);
    if(patterns) {
        for(size_t i = 0; i < patterns->len; ++i) {
            CSSPattern* pattern = patterns->items[i];
            if(css_match_pattern(pattern->items, pattern->len, tag)) {
                for(size_t j = 0; j < pattern->attribute.len; ++j) {
                    css_add_attribute(&tag->css_attribs, pattern->attribute.items[j]);
                }
            }
        }
    }
    for(size_t i = 0; i < tag->children.len; ++i) {
        match_css_patterns(tag->children.items[i], tags);
    }
}
void apply_css_styles(HTMLTag* tag) {
    for(size_t i = 0; i < tag->css_attribs.len; ++i) { 
        CSSAttribute* att = &tag->css_attribs.items[i];
        // TODO: atomise this sheizung
        if(strcmp(att->name->data, "display") == 0) {
            if(att->args.len > 1) fprintf(stderr, "WARN display argument has more arguments!\n");
            else if(att->args.len < 1) {
                fprintf(stderr, "ERROR display missing argument!\n");
                continue;
            }
            CSSArg* arg = &att->args.items[i];
            if(arg->value_len == 5 && memcmp(arg->value, "block", 5) == 0) {
                tag->display = CSSDISPLAY_BLOCK;
            } else if(arg->value_len == 6 && memcmp(arg->value, "inline", 6) == 0) {
                tag->display = CSSDISPLAY_INLINE;
            } else if(arg->value_len == 12 && memcmp(arg->value, "inline-block", 12) == 0) {
                tag->display = CSSDISPLAY_INLINE_BLOCK;
            }
        } else {
            fprintf(stderr, "WARN "__FILE__":" STRINGIFY1(__LINE__)": Unhandled attribute: `%s`\n", att->name->data);
        }
    }
    for(size_t i = 0; i < tag->children.len; ++i) {
        apply_css_styles(tag->children.items[i]);
    }
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
    bool headless = false;
    bool rawjs = false;
    while(argc) {
        const char* arg = shift_args(&argc, &argv);
        if (strcmp(arg, "--help") == 0) {
            help(stdout, exe);
            return 0;
        } else if (strcmp(arg, "--headless") == 0) headless = true;
        else if (strcmp(arg, "--rawjs") == 0) rawjs = true;
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
    if (rawjs) return run_js(content);
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
    for(size_t i = 0; i < ARRAY_LEN(block_tags); ++i) {
        Atom* atom = atom_table_get(&atom_table, block_tags[i], strlen(block_tags[i]));
        if(!atom) {
            atom = atom_new_cstr(block_tags[i]);
            atom_table_insert(&atom_table, atom);
        }
        atom_set_insert(&block_elements, atom);
    }
    const char* void_tags[] = {
        "area", "base", "br", "col", "embed", "hr", "img", "input", "link", "meta", "source", "track", "wbr"
    };
    AtomSet void_elements = { 0 };
    for(size_t i = 0; i < ARRAY_LEN(void_tags); ++i) {
        Atom* atom = atom_table_get(&atom_table, void_tags[i], strlen(void_tags[i]));
        if(!atom) {
            atom = atom_new_cstr(void_tags[i]);
            atom_table_insert(&atom_table, atom);
        }
        atom_set_insert(&void_elements, atom);
    }
    root.name = atom_new("\\root", 5);
    assert(atom_table_insert(&atom_table, root.name) && "Just buy more RAM");
    root.display = CSSDISPLAY_BLOCK;
    HTMLTag* node = &root;
    // TODO: special handling for those.
    // I couldn't give a shit for now so :(
#if 0
    const char* raw_text_elements_str[] = {
        "script", "style"
    };
    Atom* raw_text_elements[ARRAY_LEN(raw_text_elements)];
    for(size_t i = 0; i < sizeof(raw_text_elements_str)/sizeof(*raw_text_elements_str); ++i) {
        Atom* atom = atom_table_get(&atom_table, raw_text_elements_str[i], strlen(raw_text_elements_str[i]));
        if(!atom) {
            atom = atom_new_cstr(raw_text_elements_str[i]);
            atom_table_insert(&atom_table, atom);
        }
        raw_text_elements[i] = atom;
    }
#endif
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
        if (!atom_set_get(&void_elements, tag->name) && !tag->self_closing && tag->name) node = tag;
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
    if (headless) return 0;
    CSSPatternMap tags = { 0 };
    HTMLTag* html = find_child_html_tag(&root, "html");
    HTMLTag* head = find_child_html_tag(html, "head");
    Atom* style_atom = atom_table_get(&atom_table, "style", 5);
    if(style_atom && head) {
        for(size_t i = 0; i < head->children.len; ++i) {
            HTMLTag* tag = head->children.items[i];
            if(tag->name == style_atom && tag->children.len > 0) {
                const char* css_content = tag->children.items[0]->str_content;
                const char* css_content_end = tag->children.items[0]->str_content + tag->children.items[0]->str_content_len;
                for(;;) {
                    css_content = css_skip(css_content, css_content_end);
                    if(css_content >= css_content_end) break;
                    int e;
                    CSSPattern* pattern = malloc(sizeof(*pattern));
                    if(!pattern) break;
                    memset(pattern, 0, sizeof(*pattern));
                    CSSTag* tag = malloc(sizeof(*tag));
                    if(!tag) {
                        free(pattern);
                        break;
                    }
                    memset(tag, 0, sizeof(*tag));
                    pattern->items = tag;
                    pattern->len = 0;
                    pattern->cap = 1;
                    if((e=css_parse_tag(&atom_table, css_content, css_content_end, (char**)&css_content, tag))) {
                        fprintf(stderr, "CSS:ERROR %s\n", csserr_str(e));
                        break;
                    }
                    css_content = css_skip(css_content, css_content_end);
                    if(*css_content == ',') todof("Implement coma separated tags");
                    else if(isalnum(*css_content)) todof("Implement space separated tags (patterns)");
                    else if(*css_content != '{') { 
                        fprintf(stderr, "Fok you leather man: `%c`\n", *css_content);
                        goto css_end;
                    }
                    css_content++;
                    for(;;) {
                        css_content = css_skip(css_content, css_content_end);
                        if(css_content >= css_content_end) goto css_end;
                        if(*css_content == '}') {
                            css_content++;
                            break;
                        }
                        da_reserve(&pattern->attribute, 1);
                        e = css_parse_attribute(&atom_table, css_content, css_content_end, (char**)&css_content, &pattern->attribute.items[pattern->attribute.len++]);
                        if(e < 0) {
                            fprintf(stderr, "ERROR %s\n", csserr_str(e));
                            goto css_end;
                        }
                    }
                    CSSPatterns patterns = { 0 };
                    da_push(&patterns, pattern);
                    css_pattern_map_insert(&tags, tag->name, patterns);
                }
                css_end:
            }
        }
    }
    HTMLTag* body = find_child_html_tag(html, "body");
    match_css_patterns(body, &tags);
    apply_css_styles(body);
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
        if(body) {
            compute_box_html_tag(body, font, fontSize, fontSize, spacing, &x, &y);
            if(show_boxes) render_box_html_tag(body, scroll_y);
            render_html_tag(body, font, fontSize, fontSize, spacing, scroll_y);
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
