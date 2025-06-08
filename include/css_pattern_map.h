#pragma once
#include <css.h>
typedef struct CSSPatternMapBucket CSSPatternMapBucket;
struct CSSPatternMapBucket {
    CSSPatternMapBucket* next;
    // Essentially equivilent to pattern.items[0].name. We just cache it cuz its not changed
    // anyway so who cares
    Atom* name;
    CSSPattern pattern;
};
typedef struct {
    struct {
        CSSPatternMapBucket** items;
        size_t len;
    } buckets;
    size_t len;
} CSSPatternMap;
bool css_pattern_map_reserve(CSSPatternMap* map, size_t extra);
bool css_pattern_map_insert(CSSPatternMap* map, Atom* name, CSSPattern pattern);
CSSPattern* css_pattern_map_get(CSSPatternMap* map, Atom* name);
