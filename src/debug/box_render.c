#include <debug/box_render.h>
#include <html.h>
#include <raylib.h>

size_t render_box_color_n = 0;
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
    DrawRectangle(tag->x, ((float)tag->y) + scroll_y, tag->width, tag->height, colors[render_box_color_n++]);
    for(size_t i = 0; i < tag->children.len; ++i) {
        render_box_html_tag(tag->children.items[i], scroll_y);
    }
}
