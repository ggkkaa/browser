#include <stdio.h>
#include <raylib.h>

#define W_RATIO 16
#define H_RATIO 9
#define SCALE 100
#define WIDTH  W_RATIO*SCALE
#define HEIGHT H_RATIO*SCALE
int main(void) {
    InitWindow(WIDTH, HEIGHT, "Bikeshed");
    while(!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RED);
        EndDrawing();
    }
    CloseWindow();
}
