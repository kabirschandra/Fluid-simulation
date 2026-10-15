#ifndef RENDER_H
#define RENDER_H
#include <stdint.h>

uint8_t dye_intensity(float concentration);
void update_framebuffer(float *dye, uint32_t *frameBuffer);

#endif
