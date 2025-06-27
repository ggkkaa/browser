#include <css/apply.h>
#include <html.h>
#include <todo.h>
#include <atom.h>
#include <string.h>
#include <css/parse_values.h>

void apply_css_styles(HTMLTag* tag, float rootFontSize) {
    tag->fontSize = rootFontSize;
    for(size_t i = 0; i < tag->css_attribs.len; ++i) { 
        CSSAttribute* att = &tag->css_attribs.items[i];
        // TODO: atomise this sheizung
        if(strcmp(att->name->data, "display") == 0) {
            if(att->args.len > 1) fprintf(stderr, "WARN ignoring extra args to display\n");
            else if(att->args.len < 1) {
                fprintf(stderr, "CSS:ERROR too few args to display!\n");
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
        } else if (strcmp(att->name->data, "font") == 0) {
            switch(att->args.len) {
            case 3: {
                CSSArg font_size = att->args.items[0];
                CSSArg font = att->args.items[1];
                CSSArg family = att->args.items[2];
                fprintf(stderr, "CSS:TODO "__FILE__":"STRINGIFY1(__LINE__)" font: %.*s, family: %.*s\n", (int)font.value_len, font.value, (int)family.value_len, family.value);
                const char* css_start = font_size.value;
                const char* css_end = font_size.value + font_size.value_len;
                int e = css_compute_numeric(rootFontSize, css_start, css_end, &css_start, &tag->fontSize);
                if(e < 0) {
                    fprintf(stderr, "CSS:ERROR parsing font_size in font: %s\n", csserr_str(e));
                    continue;
                }
                if(css_start < css_end && css_start[0] == '/') {
                    fprintf(stderr, "CSS:TODO "__FILE__":"STRINGIFY1(__LINE__)" parse font-size/line height\n");
                    continue;
                }

            } break;
            default:
                fprintf(stderr, "CSS:WARN Ignoring %zu number of arguments to font: property\n", att->args.len);
            }
        } else if(strcmp(att->name->data, "font-size") == 0) {
            if(att->args.len > 1) fprintf(stderr, "WARN ignoring extra args to font-size\n");
            else if(att->args.len < 1) {
                fprintf(stderr, "CSS:ERROR too few args in font-size!\n");
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
            fprintf(stderr, "WARN "__FILE__":" STRINGIFY1(__LINE__)" Unhandled attribute: `%s`\n", att->name->data);
        }
    }
    for(size_t i = 0; i < tag->children.len; ++i) {
        apply_css_styles(tag->children.items[i], rootFontSize);
    }
}
