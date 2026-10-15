#include "input.h"
#include "grid.h"
#include "config.h"
#include <math.h>

/** Add mouse-directed velocity inside a circular brush, fading toward its edge. */
void inject_velocity(float *u, float *v, float mouseX, float mouseY, float dx, float dy) {

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
                float falloff = 1.0f - sqrtf(distSquared) / MOUSE_RADIUS;
                u[i] += dx * MOUSE_STRENGTH * falloff;
                v[i] += dy * MOUSE_STRENGTH * falloff;
            }

        }
    }

    return;
}
