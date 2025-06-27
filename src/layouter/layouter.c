#include <html.h>
#include <css/pattern_map.h>
#include <todo.h>
#include <assert.h>
#include <string.h>
#include <raylib.h>
#include <atom.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
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
