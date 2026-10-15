#include "render.h"
#include "grid.h"
#include "config.h"

/** Convert dye concentration [0,1] into an 8-bit grayscale intensity. */
uint8_t dye_intensity(float concentration) {

    concentration = clampf(concentration, 0.0f, 1.0f);

    return (uint8_t)(255 * concentration);
}

/** Convert the dye array into XRGB8888 pixels for the SDL texture. */
void update_framebuffer(float *dye, uint32_t *frameBuffer) {
    for(int i = 0; i < WIDTH * HEIGHT; i++) {
        uint8_t intensity = dye_intensity(dye[i]);

        frameBuffer[i] = (intensity << 16) | (intensity << 8) | intensity;
    }

    return;
}
