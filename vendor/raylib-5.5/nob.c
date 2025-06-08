#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

static const char *raylib_modules[] = {
    "rcore",
    "raudio",
    "rglfw",
    "rmodels",
    "rshapes",
    "rtext",
    "rtextures",
    "utils",
};

#define RAYLIB_SRC_FOLDER "vendor/raylib-5.5/src/"

int main(int argc, char** argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
    char* cc = getenv("CC");
    // TODO: automatic checks for the compiler 
    // available on the system. Maybe default to clang on bimbows
    if(!cc) cc = "cc";
    setenv("CC", cc, 0);
    char* bindir = getenv("BINDIR");
    if(!bindir) bindir = "bin";
    setenv("BINDIR", bindir, 0);
    if(!mkdir_if_not_exists(bindir)) return 1;

    const char* build_path = temp_sprintf("%s/raylib", bindir);

    if(!mkdir_if_not_exists(build_path)) return 1;

    Cmd cmd = {0};

    File_Paths object_files = {0};
    Procs procs = {0};

    for (size_t i = 0; i < NOB_ARRAY_LEN(raylib_modules); ++i) {
        const char *input_path = temp_sprintf(RAYLIB_SRC_FOLDER"%s.c", raylib_modules[i]);
        const char *output_path = temp_sprintf("%s/%s.o", build_path, raylib_modules[i]);
        output_path = temp_sprintf("%s/%s.o", build_path, raylib_modules[i]);

        da_append(&object_files, output_path);

        if (needs_rebuild(output_path, &input_path, 1)) {
            cmd_append(&cmd, cc,
                "-ggdb", "-DPLATFORM_DESKTOP", "-D_GLFW_X11", "-fPIC", "-DSUPPORT_FILEFORMAT_FLAC=1",
                "-I"RAYLIB_SRC_FOLDER"external/glfw/include",
                "-c", input_path,
                "-o", output_path);
            da_append(&procs, cmd_run_async_and_reset(&cmd));
        }
    }

    if (!procs_wait_and_reset(&procs)) return 1;

    const char *libraylib_path = temp_sprintf("%s/libraylib.a", build_path);

    if (needs_rebuild(libraylib_path, object_files.items, object_files.count)) {
        cmd_append(&cmd, "ar", "-crs", libraylib_path);
        for (size_t i = 0; i < ARRAY_LEN(raylib_modules); ++i) {
            const char *input_path = temp_sprintf("%s/%s.o", build_path, raylib_modules[i]);
            cmd_append(&cmd, input_path);
        }
        if (!cmd_run_sync_and_reset(&cmd)) return 1;
    }

    return 0;
}