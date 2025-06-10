#include "html.h"
#include "assert.h"

static_assert(HTMLERR_COUNT == 5, "Update htmlerr_strtab");
static const char* htmlerr_strtab[] = {
    [HTMLERR_TODO] = "Unimplemented",
    [HTMLERR_EOF]  = "End of File",
    [HTMLERR_INVALID_TAG] = "Invalid tag format",
    [HTMLERR_INVALID_ATTRIBUTE]  = "Invalid attribute format",
};
const char* htmlerr_str(int err) {
    if(err >= 0) return "OK";
    err = -err;
    if(err >= HTMLERR_COUNT) return "Unknown error";
    return htmlerr_strtab[err];
}
