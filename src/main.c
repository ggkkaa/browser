#include <stdio.h>
#include <raylib.h>
#include <fileutils.h>

#define W_RATIO 16
#define H_RATIO 9
#define SCALE 100
#define WIDTH  W_RATIO*SCALE
#define HEIGHT H_RATIO*SCALE
int main(void) {
    // TODO: Unhardcode this sheizung
    const char* example_path = "examples/barebones.html";
    size_t content_size;
    const char* content = read_entire_file(example_path, &content_size);
    InitWindow(WIDTH, HEIGHT, "Bikeshed");
    while(!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText(content, 0, 0, 0, BLACK);
        EndDrawing();
    }
    CloseWindow();
}
