#include "grid.h"
#include "config.h"
#include <math.h>

/** Linear interpolation between a and b. t=0 gives a; t=1 gives b. */
float lerp(float a, float b, float t) {

    return ((1-t)*a) + (t*b);
}

/** Interpolate four neighbouring cell values. Used when a backtraced position falls between cells. */
float bilinear(float A, float B, float C, float D, float fx, float fy) {

    float l1 = lerp(A, B, fx);
    float l2 = lerp(C, D, fx);
    float l3 = lerp(l1, l2, fy);

    return l3;
}

/** Keep a float within the given inclusive range. */
float clampf(float value, float min, float max) {

    return fminf(fmaxf(value, min), max);
}

/** Return the larger integer. */
int maxi(int a, int b) {
    return (a > b) ? a : b;
}

/** Return the smaller integer. */
int mini(int a, int b) {
    return (a < b) ? a : b;
}

/** Keep an integer within the given inclusive range. */
int clampi(int value, int min, int max) {

    return mini(maxi(value, min), max);
}

/** Read a grid at floating-point coordinates using bilinear interpolation. Coordinates are clamped to the grid, so sampling never reads outside an array. */
float sample(float *grid, float x, float y) {

    x = clampf(x, 0.0f, WIDTH - 1);
    y = clampf(y , 0.0f, HEIGHT - 1);

    int x0 = floorf(x), x1 = x0 + 1;
    int y0 = floorf(y), y1 = y0 + 1;

    float fx = x - x0;
    float fy = y - y0;

    //Secondary clamp
    x1 = clampi(x1, 0, WIDTH - 1);
    y1 = clampi(y1, 0, HEIGHT - 1);

    float A = grid[y0 * WIDTH + x0];
    float B = grid[y0 * WIDTH + x1];
    float C = grid[y1 * WIDTH + x0];
    float D = grid[y1 * WIDTH + x1];

    return bilinear(A, B, C, D, fx ,fy);
}

/** Swap the current and destination buffers without copying WIDTH*HEIGHT floats. */
void swap_pointers(float **p1, float **p2) {

    float *temp = *p1;

    *p1 = *p2;
    *p2 = temp;

    return;
}
