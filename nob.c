#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

int main(int argc, char** argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
    char* cc = getenv("CC");
    // TODO: automatic checks for the compiler 
    // available on the system. Maybe default to clang on bimbows
    if(!cc) cc = "cc";
    char* bindir = getenv("BINDIR");
    if(!bindir) bindir = "bin";
    if(!mkdir_if_not_exists(bindir)) return 1;
    if(!mkdir_if_not_exists(temp_sprintf("%s/bikeshed", bindir))) return 1;
}
