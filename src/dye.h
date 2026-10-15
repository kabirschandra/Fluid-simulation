#ifndef DYE_H
#define DYE_H

void advect_dye(float *oldDye, float *newDye, float *u, float *v, float dt);
void inject_dye(float *dye, float mouseX, float mouseY);
void init_dye(float *dye);

#endif
