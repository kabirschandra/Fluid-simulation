#include "fluid.h"
#include "grid.h"
#include "config.h"
#include <math.h>

/** Set both velocity components to zero along all four walls (simple no-slip walls). */
void velocity_boundaries(float *u, float *v) {
    // Left and right walls
    for(int y = 1; y < HEIGHT - 1; y++) {

        int left = y * WIDTH;
        int right = y * WIDTH + WIDTH - 1;

        u[left] = 0.0f;
        u[right] = 0.0f;

        v[left] = 0.0f;
        v[right] = 0.0f;
    }

    // Top and bottom walls
    for(int x = 1; x < WIDTH - 1; x++) {

        int top = x;
        int bottom = (HEIGHT - 1) * WIDTH + x;

        u[top] = 0.0f;
        u[bottom] = 0.0f;

        v[top] = 0.0f;
        v[bottom] = 0.0f;
    }

    //Corners
    u[0] = 0.0f;
    u[WIDTH - 1] = 0.0f;
    u[(HEIGHT - 1) * WIDTH] = 0.0f;
    u[HEIGHT * WIDTH - 1] = 0.0f;

    v[0] = 0.0f;
    v[WIDTH - 1] = 0.0f;
    v[(HEIGHT - 1) * WIDTH] = 0.0f;
    v[HEIGHT * WIDTH - 1] = 0.0f;

    return;
}

/** Subtract the pressure gradient from velocity: velocity -= dt * grad(pressure). This is the projection step; walls are reset afterwards. */
void project_velocity(float *oldU, float *oldV, float *newU, float *newV, float *p, float dt) {

    for(int y = 1; y < HEIGHT - 1; y++) {
        for(int x = 1; x < WIDTH - 1; x++) {

            int i = y * WIDTH + x;

            float dp_dx = (p[i + 1] - p[i - 1]) / 2.0f;
            float dp_dy = (p[i + WIDTH] - p[i - WIDTH]) / 2.0f;

            newU[i] = oldU[i] - dt * dp_dx;
            newV[i] = oldV[i] - dt * dp_dy;

        }
    }

    velocity_boundaries(newU, newV);
    return;
}

/** Copy neighbouring pressure values onto the walls (approximate zero normal pressure gradient). */
void pressure_boundaries(float *p) {

    // Left and right walls
    for(int y = 1; y < HEIGHT - 1; y++) {

        int left = y * WIDTH;
        int right = y * WIDTH + WIDTH - 1;

        p[left] = p[left + 1];
        p[right] = p[right - 1];
    }

    // Top and bottom walls
    for(int x = 1; x < WIDTH - 1; x++) {

        int top = x;
        int bottom = (HEIGHT - 1) * WIDTH + x;

        p[top] = p[top + WIDTH];
        p[bottom] = p[bottom - WIDTH];
    }

    //Corners
    p[0] = p[WIDTH + 1];
    p[WIDTH - 1] = p[2 * WIDTH - 2];
    p[(HEIGHT - 1) * WIDTH] = p[(HEIGHT - 2) * WIDTH + 1];
    p[HEIGHT * WIDTH - 1] = p[(HEIGHT - 2) * WIDTH + WIDTH - 2];

    return;
}

/** Compute du/dx + dv/dy with centred differences. Nonzero divergence indicates local expansion/compression in the discrete velocity field. */
void calculate_divergence(float *u, float *v, float *divergence) {

    for(int y = 1; y < HEIGHT - 1; y++) {
        for(int x = 1; x < WIDTH - 1; x++) {
            float du_dx = (u[y * WIDTH + (x + 1)] - u[y * WIDTH + (x - 1)]) / 2.0f;

            float dv_dy = (v[(y + 1) * WIDTH + x] - v[(y - 1) * WIDTH + x]) / 2.0f;

            divergence[y * WIDTH + x] = du_dx + dv_dy;
        }
    }

    return;
}

/** Perform one Jacobi pressure iteration using the existing two-cell stencil. It matches the interior composition of centred divergence/gradient, but near-wall cells are not solved and odd/even grid modes can decouple. This is an approximate solver, not a validated incompressible projection. */
void pressure_iteration(float *oldP, float *newP, float *divergence, float dt) {

    for(int y = 2; y < HEIGHT - 2; y++) {
        for(int x = 2; x < WIDTH - 2; x++) {

            int i = y * WIDTH + x;

            float l = oldP[i - 2];
            float r = oldP[i + 2];
            float t = oldP[i - 2 * WIDTH];
            float b = oldP[i + 2 * WIDTH];

            newP[i] = (l + r + t + b - 4.0f * divergence[i] / dt) / 4.0f;
        }
    }

    return;
}

/** Move the velocity field along itself using semi-Lagrangian backtracing. Each destination cell samples where its fluid came from one time step earlier. */
void advect_velocity(float *oldU, float *oldV, float *newU, float *newV, float dt) {
    for(int y = 0; y < HEIGHT; y++) {

        for(int x = 0; x < WIDTH; x++) {
            int i = y * WIDTH + x;

            float sourceX = x - oldU[i] * dt;
            float sourceY = y - oldV[i] * dt;

            newU[i] = sample(oldU, sourceX, sourceY);
            newV[i] = sample(oldV, sourceX, sourceY);
        }
    }
    return;
}

/** Calculate 2D curl (dv/dx - du/dy). Positive and negative values indicate opposite rotation directions. */
void calculate_vorticity(float *u, float *v, float *vorticity) {

    for(int y = 1; y < HEIGHT - 1; y++) {
        for(int x = 1; x < WIDTH - 1; x++) {

            int i = y * WIDTH + x;

            float dv_dx = (v[i + 1] - v[i - 1]) / 2.0f;
            float du_dy = (u[i + WIDTH] - u[i - WIDTH]) / 2.0f;

            vorticity[i] = dv_dx - du_dy;
        }
    }

    return;
}

/** Apply an explicit viscosity step using a five-point Laplacian. With VISCOSITY=0 this copies the interior unchanged and resets wall velocities. */
void diffuse_velocity(float *oldU, float *oldV, float *newU, float *newV, float viscosity, float dt) {

    // Top and bottom boundaries
    for(int x = 0; x < WIDTH; x++) {
        int top = x;
        int bottom = (HEIGHT - 1) * WIDTH + x;

        newU[top] = 0.0f;
        newV[top] = 0.0f;

        newU[bottom] = 0.0f;
        newV[bottom] = 0.0f;
    }

    // Left and right boundaries
    for(int y = 0; y < HEIGHT; y++) {
        int left = y * WIDTH;
        int right = y * WIDTH + WIDTH - 1;

        newU[left] = 0.0f;
        newV[left] = 0.0f;

        newU[right] = 0.0f;
        newV[right] = 0.0f;
    }

    for(int y = 1; y < HEIGHT - 1; y++) {
        for(int x = 1; x < WIDTH - 1; x++) {

            int i = y * WIDTH + x;
        
            float laplacianU =
                oldU[i - 1] +
                oldU[i + 1] +
                oldU[i - WIDTH] +
                oldU[i + WIDTH] -
                4.0f * oldU[i];

            float laplacianV =
                oldV[i - 1] +
                oldV[i + 1] +
                oldV[i - WIDTH] +
                oldV[i + WIDTH] -
                4.0f * oldV[i];

            newU[i] = oldU[i] + viscosity * dt * laplacianU;
            newV[i] = oldV[i] + viscosity * dt * laplacianV;
        }
    }

    return;
}

/** Return the average absolute divergence over interior cells for debugging. */
float average_divergence(float *divergence) {

    float total = 0.0f;
    int count = 0;

    for(int y = 1; y < HEIGHT - 1; y++) {
        for(int x = 1; x < WIDTH - 1; x++) {

            int i = y * WIDTH + x;

            total += fabsf(divergence[i]);
            count++;
        }
    }

    return total / count;
}

/** Add a rotational force toward regions of stronger curl. This counteracts the numerical smoothing caused by semi-Lagrangian advection; it is a visual enhancement, not a physical viscosity term. */
void apply_vorticity_confinement(float *u, float *v, float *vorticity, float dt) {
    
    for(int y = 2; y < HEIGHT - 2; y++) {
        for(int x = 2; x < WIDTH - 2; x++) {

            int i = y * WIDTH + x;

            // Gradient of absolute vorticity
            float gx = (
                fabsf(vorticity[i + 1]) -
                fabsf(vorticity[i - 1])
            ) / 2.0f;

            float gy = (
                fabsf(vorticity[i + WIDTH]) -
                fabsf(vorticity[i - WIDTH])
            ) / 2.0f;

            // Normalise gradient
            float length = sqrtf(gx * gx + gy * gy);

            if(length < 1e-6f)
                continue;

            float nx = gx / length;
            float ny = gy / length;

            // Vorticity confinement force
            float fx = VORTICITY_STRENGTH * ny * vorticity[i];
            float fy = -VORTICITY_STRENGTH * nx * vorticity[i];

            // Apply force to velocity
            u[i] += fx * dt;
            v[i] += fy * dt;
        }
    }

    return;
}

/** Optional test field: set every cell to a uniform velocity. */
void init_velocity(float *u, float *v) {
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            u[y * WIDTH + x] = 60.0f;
            v[y * WIDTH + x] = 60.0f;
        }
    }

    return;
}

