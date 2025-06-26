#pragma once
// TODO: Move this out
#include <raylib.h>
Vector2 MeasureCodepointEx(Font font, int codepoint, float fontSize, float spacing);

#include <stddef.h>
typedef struct HTMLTag HTMLTag;
void compute_box_html_tag(HTMLTag* tag, Font font, float fontSize, float textFontSize, float spacing, size_t* cursor_x, size_t* cursor_y);
