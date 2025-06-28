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
#include <html.h>
#include <css/pattern_map.h>
#include <css/parse_values.h>
#include <css/match.h>
#include <css/apply.h>
#include <layouter/layouter.h>
#include <debug/box_render.h>
#include <render/html_tag.h>
#include <fixup.h>


#define W_RATIO 16
#define H_RATIO 9
#define SCALE 100
#define WIDTH  W_RATIO*SCALE
#define HEIGHT H_RATIO*SCALE
#define ARRAY_LEN(a) (sizeof(a)/sizeof(*a))



HTMLTag* find_child_html_tag(HTMLTag* tag, const char* name) {
    if(!tag) return NULL;
    size_t name_len = strlen(name);
    for(size_t i = 0; i < tag->children.len; ++i) {
        HTMLTag* child = tag->children.items[i];
        if(child->name->len == name_len && memcmp(child->name->data, name, name_len) == 0) return child;
    }
    return NULL;
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
            tag->id = atom_table_get(atom_table, att->value, att->value_len);
            if(!tag->id) {
                atom_table_insert(atom_table, atom_new(att->value, att->value_len));
                tag->id = atom_table_get(atom_table, att->value, att->value_len);
            }
        } else if(tag->attributes.items[i]->key_len == 5 && memcmp(tag->attributes.items[i]->key, "class", 5) == 0) {
            HTMLAttribute* att = tag->attributes.items[i];
            tag->class = atom_table_get(atom_table, att->value, att->value_len);
            if(!tag->class) {
                atom_table_insert(atom_table, atom_new(att->value, att->value_len));
                tag->class = atom_table_get(atom_table, att->value, att->value_len);
            }
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
    match_css_patterns(html, &selector_maps);
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
    apply_css_styles(html, fontSize);
    fixup_tree(body);
    dump_html_tag(node, 0);
    if (headless) {
        CloseWindow();
        return 0;
    }
    SetTargetFPS(60);
    float scroll_y = 0;
    bool show_boxes = false;
    if(!html->background_color) html->background_color = ColorToInt(RAYWHITE);
    fprintf(stderr, "html->background_color=%08X\n", html->background_color);
    while(!WindowShouldClose()) {
        scroll_y += GetMouseWheelMove()*16.0;
        if(IsKeyReleased(KEY_F4)) show_boxes = !show_boxes;
        BeginDrawing();
        ClearBackground(GetColor(html->background_color));
        
        size_t x = 0, y = 0;
        render_box_color_n = 0;
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
