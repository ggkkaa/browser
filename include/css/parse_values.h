#pragma once
int css_parse_float(const char* css_content, const char* css_content_end, const char** end, float* result);
int css_compute_numeric(float rootFontSize, const char* css_content, const char* css_content_end, const char** end, float* result);
