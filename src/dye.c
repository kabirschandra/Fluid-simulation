#include "dye.h"
#include "grid.h"
#include "config.h"
#include <math.h>

/** Move dye through the corrected velocity field and multiply by DYE_DISSIPATION each frame. */
void advect_dye(float *oldDye, float *newDye, float *u, float *v, float dt) {
    for(int y = 0; y < HEIGHT; y++) {

        for(int x = 0; x < WIDTH; x++) {
            int i = y * WIDTH + x;

            float sourceX = x - u[i] * dt;
            float sourceY = y - v[i] * dt;

            newDye[i] = sample(oldDye, sourceX, sourceY) * DYE_DISSIPATION;
        }
    }
    return;
}

/** Add white dye inside the mouse brush circle, capped at concentration 1. */
void inject_dye(float *dye, float mouseX, float mouseY) {

    int yStart = clampi(mouseY - MOUSE_RADIUS, 0, HEIGHT - 1);
    int yEnd = clampi(mouseY + MOUSE_RADIUS, 0, HEIGHT - 1);

    int xStart = clampi(mouseX - MOUSE_RADIUS, 0, WIDTH - 1);
    int xEnd = clampi(mouseX + MOUSE_RADIUS, 0, WIDTH - 1);

    for(int y = yStart; y <= yEnd; y++) {
        for(int x = xStart; x <= xEnd; x++) {

            int i = (y * WIDTH) + x;

            float distX = x - mouseX;
            float distY = y - mouseY;
            float distSquared = distX * distX + distY * distY;

            if(MOUSE_RADIUS * MOUSE_RADIUS >= distSquared) {
                dye[i] = fminf(dye[i] + 0.1f, 1.0f);
            }

        }
    }

    return;
}

/** Optional test pattern: a 20x20 white square near the upper-left of the grid. */
void init_dye(float *dye) {

    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {

            int i = y * WIDTH + x;

            if((x >= 40 && x <= 59) && (y >= 40 && y <= 59)) {
                dye[i] = 1.0f;
            } else {
                dye[i] = 0.0f;
            }
        }
    }

    return;
}
