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
            if(child->display == CSSDISPLAY_BLOCK) {
                new_x = tag->x;
                new_y = max_y;
            }
            compute_box_html_tag(child, font, fontSize, tag->fontSize, spacing, &new_x, &new_y);
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
            if(x + size.x > max_x) max_x = ceilf(x + size.x);
            if(y + size.y > max_y) max_y = ceilf(y + size.y);
            x += size.x;
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
            render_html_tag(tag->children.items[i], font, fontSize, tag->fontSize, spacing, scroll_y);
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

void fixup_tree(HTMLTag* tag){
    /* 
        if we encounter blocks inside we need to get them out of children put them
        after this tag and if there are any spare inline blocks left then create clone
        of this tag that has rest of spare tags 
    */
    for(size_t i = 0; i < tag->children.len; i++){
        HTMLTag* child = tag->children.items[i];
        fixup_tree(child);
        
        HTMLTag* clone = NULL;
        if(child->display == CSSDISPLAY_INLINE){
            for(size_t j = 0; j < child->children.len; j++){
                if(child->children.items[j]->display == CSSDISPLAY_BLOCK){
                    if(j == 0){
                        da_insert(&tag->children, i, child->children.items[j]);
                        memmove(&child->children.items[0], &child->children.items[1], (--child->children.len)*sizeof(*child->children.items));
                    }else if(j < child->children.len-1){ // create clone
                        clone = malloc(sizeof(HTMLTag));
                        memcpy(clone, child, sizeof(HTMLTag));
                        clone->children.items = NULL;
                        clone->children.cap = 0;
                        clone->children.len = 0;
                        
                        for(size_t m = j+1; m < child->children.len; m++){
                            da_push(&clone->children,child->children.items[m]);
                        }
                        child->children.len = j;
                        da_insert(&tag->children, i+1, child->children.items[j]);
                    }else{
                        da_insert(&tag->children, i+1, child->children.items[j]);
                        child->children.len--;
                    }
                    break;
                }
            }
            if(clone) da_insert(&tag->children, i+2, clone);
        }
    }
}

void match_css_patterns(HTMLTag* tag, CSSPatternMaps* selector_maps) {
    CSSPatterns* patterns = css_pattern_map_get(&selector_maps->maps[CSSTAG_TAG], tag->name);
    if(patterns) {
        for(size_t i = 0; i < patterns->len; ++i) {
            CSSPattern* pattern = &patterns->items[i];
            if(css_match_pattern(pattern->items, pattern->len, tag)) {
                for(size_t j = 0; j < pattern->attributes.len; ++j) {
                    css_add_attribute(&tag->css_attribs, pattern->attributes.items[j]);
                }
            }
        }
    }
    patterns = css_pattern_map_get(&selector_maps->maps[CSSTAG_ID], tag->id);
    if(patterns) {
        for(size_t i = 0; i < patterns->len; ++i) {
            CSSPattern* pattern = &patterns->items[i];
            if(css_match_pattern(pattern->items, pattern->len, tag)) {
                for(size_t j = 0; j < pattern->attributes.len; ++j) {
                    css_add_attribute(&tag->css_attribs, pattern->attributes.items[j]);
                }
            }
        }
    }
    for(size_t i = 0; i < tag->children.len; ++i) {
        match_css_patterns(tag->children.items[i], selector_maps);
    }
}
int css_parse_float(const char* css_content, const char* css_content_end, const char** end, float* result) {
    float sign = 1.f;
    if(css_content < css_content_end && *css_content == '-') {
        sign = -sign;
        css_content++;
    }
    float value = 0.f;
    while(css_content < css_content_end && isdigit(*css_content)) {
        value = value * 10.f + (*css_content - '0');
        css_content++;
    }
    float decimal = 0.1f;
    if(css_content < css_content_end && *css_content == '.') {
        css_content++;
        while(css_content < css_content_end && isdigit(*css_content)) {
            value += (*css_content - '0') * decimal;
            css_content++;
            decimal *= 0.1f;
        }
    }
    *result = value * sign;
    *end = css_content;
    return 0;
}
int css_compute_numeric(float rootFontSize, const char* css_content, const char* css_content_end, const char** end, float* result) {
    int e;
    *result = 0.0;
    float number;
    if((e=css_parse_float(css_content, css_content_end, &css_content, &number)) < 0) return e;
    const char* unit_str = css_content;
    while(css_content < css_content_end && isalnum(*css_content)) css_content++;
    size_t unit_len = css_content - unit_str;
    if(unit_len == 3 && memcmp(unit_str, "rem", 3) == 0) {
        *result = number * rootFontSize;
    } else if (unit_len) {
        fprintf(stderr, "CSS:WARN "__FILE__":"STRINGIFY1(__LINE__)" Unsupported `%.*s`\n", (int)unit_len, unit_str);
    }
    *end = css_content;
    return 0;
}
void apply_css_styles(HTMLTag* tag, float rootFontSize) {
    tag->fontSize = rootFontSize;
    for(size_t i = 0; i < tag->css_attribs.len; ++i) { 
        CSSAttribute* att = &tag->css_attribs.items[i];
        // TODO: atomise this sheizung
        if(strcmp(att->name->data, "display") == 0) {
            if(att->args.len > 1) fprintf(stderr, "WARN ignoring extra args to display\n");
            else if(att->args.len < 1) {
                fprintf(stderr, "ERROR too few args to display!\n");
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
        } else if(strcmp(att->name->data, "font-size") == 0) {
            if(att->args.len > 1) fprintf(stderr, "WARN ignoring extra args to font-size\n");
            else if(att->args.len < 1) {
                fprintf(stderr, "ERROR too few args in font-size!\n");
                continue;
            }
            CSSArg* arg = &att->args.items[0];
            const char* _end;
            int e = css_compute_numeric(rootFontSize, arg->value, arg->value+arg->value_len, &_end, &tag->fontSize);
            if(e < 0) {
                fprintf(stderr, "CSS:ERROR parsing numeric: %s\n", csserr_str(e));
                continue;
            }
        } else {
            fprintf(stderr, "WARN "__FILE__":" STRINGIFY1(__LINE__)": Unhandled attribute: `%s`\n", att->name->data);
        }
    }
    for(size_t i = 0; i < tag->children.len; ++i) {
        apply_css_styles(tag->children.items[i], rootFontSize);
    }
}
int css_parse(AtomTable* atom_table, CSSPatternMaps* selector_maps, const char* css_content, const char* css_content_end, const char** end) {
    for(;;) {
        css_content = css_skip(css_content, css_content_end);
        if(css_content >= css_content_end) break;
        int e;
        CSSPatterns patterns = { 0 };
        if((e=css_parse_patterns(atom_table, &patterns, css_content, css_content_end, &css_content)) < 0) {
            // fprintf(stderr, "CSS:ERROR: parsing patterns: %s\n", csserr_str(e));
            return e;
        }
        if(css_content >= css_content_end || *css_content != '{') {
            // fprintf(stderr, "CSS:WARN: fok you leather man: `%c`\n", *css_content);
            return -CSSERR_INVALID_ATTRIBUTE_SYNTAX;
        }
        css_content++;
        for(;;) {
            css_content = css_skip(css_content, css_content_end);
            if(css_content >= css_content_end) goto css_end;
            if(*css_content == '}') {
                css_content++;
                break;
            }
            CSSAttribute att = { 0 };
            e = css_parse_attribute(atom_table, css_content, css_content_end, (char**)&css_content, &att);
            for(size_t i = 0; i < patterns.len; ++i) {
                da_push(&patterns.items[i].attributes, att);
            }
            if(e < 0) {
                // fprintf(stderr, "ERROR %s\n", csserr_str(e));
                return e;
            }
        }
        for(size_t i = 0; i < patterns.len; ++i) {
            CSSPattern* pattern = &patterns.items[i];
            CSSTag* tag = &pattern->items[0];
            da_push(css_pattern_map_get_or_insert_empty(&selector_maps->maps[tag->kind], tag->name), *pattern);
        }
    }
css_end:
    *end = css_content;
    return 0;
}
static void set_id_and_class_fields(AtomTable* atom_table, HTMLTag* tag) {
    for (size_t i = 0; i < tag->attributes.len; ++i) {
        if(tag->attributes.items[i]->key_len == 2 && memcmp(tag->attributes.items[i]->key, "id", 2) == 0) {
            HTMLAttribute* att = tag->attributes.items[i];
            atom_table_insert(atom_table, atom_new(att->value, att->value_len));
            tag->id = atom_table_get(atom_table, att->value, att->value_len);
        } else if(tag->attributes.items[i]->key_len == 5 && memcmp(tag->attributes.items[i]->key, "class", 5) == 0) {
            HTMLAttribute* att = tag->attributes.items[i];
            atom_table_insert(atom_table, atom_new(att->value, att->value_len));
            tag->class = atom_table_get(atom_table, att->value, att->value_len);
        }
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
    CSSPatternMaps selector_maps = { 0 };
    {
        const char* default_css_path = "default.css";
        size_t default_css_size;
        const char* default_css = read_entire_file(default_css_path, &default_css_size);
        if(!default_css) {
            fprintf(stderr, "ERROR: Failed to load `%s`\n", default_css_path);
            return 1;
        }
        const char* default_css_endptr;
        int e = css_parse(&atom_table, &selector_maps, default_css, default_css+default_css_size, &default_css_endptr);
        if(e < 0) {
            fprintf(stderr, "ERROR: Failed to parse default.css: %s\n", csserr_str(e)); 
            return 1;
        }
        // FIXME: clone everything so we can do:
        // free((void*)default_css)
    }
    HTMLTag* html = find_child_html_tag(&root, "html");
    HTMLTag* head = find_child_html_tag(html, "head");
    Atom* style_atom = atom_table_get(&atom_table, "style", 5);
    if(style_atom && head) {
        for(size_t i = 0; i < head->children.len; ++i) {
            HTMLTag* tag = head->children.items[i];
            if(tag->name == style_atom && tag->children.len > 0) {
                const char* css_content = tag->children.items[0]->str_content;
                const char* css_content_end = tag->children.items[0]->str_content + tag->children.items[0]->str_content_len;
                int e = css_parse(&atom_table, &selector_maps, css_content, css_content_end, &css_content);
                if(e < 0) {
                    fprintf(stderr, "CSS:ERROR parsing CSS: %s\n", csserr_str(e));
                }
            }
        }
    }
    HTMLTag* body = find_child_html_tag(html, "body");
    set_id_and_class_fields(&atom_table, body);
    match_css_patterns(body, &selector_maps);
    if (headless) return 0;
    HTMLTag* title = find_child_html_tag(head, "title");
    const char* window_title = "Bikeshed";
    if(title && title->children.len && !title->children.items[0]->name) {
        HTMLTag* title_str = title->children.items[0];
        window_title = TextFormat("Bikeshed - %.*s", (int)title_str->str_content_len, title_str->str_content);
    }
    InitWindow(WIDTH, HEIGHT, window_title);
    Font font = LoadFont("fonts/iosevka/iosevka-bold.ttf");
    float fontSize = 24.0f;
    float spacing = fontSize*0.1f;
    if (font.texture.id == 0) {
        font = GetFontDefault();
        fontSize = 18.0f;
        spacing = fontSize*0.1f;
    }
    // TODO: fix headless mode
    apply_css_styles(body, fontSize);
    fixup_tree(body);
    dump_html_tag(node, 0);
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
