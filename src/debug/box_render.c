#include <debug/box_render.h>
#include <html.h>
#include <stdio.h>

size_t render_box_color_n = 0;
void render_box_html_tag(BSRenderer* renderer, HTMLTag* tag, float scroll_y) {
#if 0
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
    DrawRectangle(tag->x, ((float)tag->y) + scroll_y, tag->width, tag->height, colors[render_box_color_n++]);
    for(size_t i = 0; i < tag->children.len; ++i) {
        render_box_html_tag(tag->children.items[i], scroll_y);
    }
#else
    // TODO: port render_box_html_tag to the generic renderer. I'm too lazy to unhardcode the raylib colors
    fprintf(stderr, "<TBD port render_box_html_tag>\n");
    (void)scroll_y;
    (void)tag;
    (void)renderer;
#endif
}
