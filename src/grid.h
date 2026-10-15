#ifndef GRID_H
#define GRID_H

float lerp(float a, float b, float t);
float bilinear(float A, float B, float C, float D, float fx, float fy);
float clampf(float value, float min, float max);
int maxi(int a, int b);
int mini(int a, int b);
int clampi(int value, int min, int max);
float sample(float *grid, float x, float y);
void swap_pointers(float **p1, float **p2);

#endif
