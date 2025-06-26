#pragma once
#include <stddef.h>
extern size_t render_box_color_n;
typedef struct HTMLTag HTMLTag;

void render_box_html_tag(HTMLTag* tag, float scroll_y);
