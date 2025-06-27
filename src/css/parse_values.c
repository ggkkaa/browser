#include <css/parse_values.h>
#include <todo.h>
#include <string.h>
#include <ctype.h>

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
