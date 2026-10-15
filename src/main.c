#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <SDL3/SDL.h>

#include "config.h"
#include "dye.h"
#include "fluid.h"
#include "grid.h"
#include "input.h"
#include "render.h"

// Two buffers per moving field: read from one, write to the other, then swap.
// Static arrays start at zero, so the fluid and dye are initially at rest.
static float uA[WIDTH * HEIGHT];
static float vA[WIDTH * HEIGHT];
static float uB[WIDTH * HEIGHT];
static float vB[WIDTH * HEIGHT];
static float dyeA[WIDTH * HEIGHT];
static float dyeB[WIDTH * HEIGHT];
static float divergence[WIDTH * HEIGHT];
static float pressureA[WIDTH * HEIGHT];
static float pressureB[WIDTH * HEIGHT];
static float vorticity[WIDTH * HEIGHT];
static uint32_t frameBuffer[WIDTH * HEIGHT];

int main(void) {
    if(!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL Init failure: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Fluid Simulation", WIDTH * STRETCH, HEIGHT * STRETCH, 0
    );

    if(!window) {
        fprintf(stderr, "SDL Window failure: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);

    if(!renderer) {
        fprintf(stderr, "SDL Renderer failure: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture *texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING,
        WIDTH, HEIGHT
    );

    if(!texture) {
        fprintf(stderr, "SDL Texture failure: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    // Optional test initial conditions:
    // init_velocity(uA, vA);
    // init_dye(dyeA);

    float *d1 = dyeA;
    float *d2 = dyeB;
    float *u1 = uA;
    float *u2 = uB;
    float *v1 = vA;
    float *v2 = vB;
    float *p1 = pressureA;
    float *p2 = pressureB;

    float dt = TIMESTEP_SCALE * (1.0f / FPS);
    bool running = true;
    const double targetFrameTime = 1.0 / FPS;

    while(running) {
        uint64_t start = SDL_GetPerformanceCounter();
        SDL_Event event;

        // Left click + drag adds dye and velocity in the direction of the mouse.
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_EVENT_QUIT) {
                running = false;
            }

            if(event.type == SDL_EVENT_MOUSE_MOTION) {
                if(event.motion.state & SDL_BUTTON_LMASK) {
                    float gridX = event.motion.x / STRETCH;
                    float gridY = event.motion.y / STRETCH;
                    float gridDX = event.motion.xrel / STRETCH;
                    float gridDY = event.motion.yrel / STRETCH;

                    inject_velocity(u1, v1, gridX, gridY, gridDX, gridDY);
                    inject_dye(d1, gridX, gridY);
                }
            }
        }

        // 1. Carry velocity through the previous velocity field.
        advect_velocity(u1, v1, u2, v2, dt);
        swap_pointers(&u1, &u2);
        swap_pointers(&v1, &v2);

        // 2. Smooth velocity according to viscosity (zero by default).
        diffuse_velocity(u1, v1, u2, v2, VISCOSITY, dt);
        swap_pointers(&u1, &u2);
        swap_pointers(&v1, &v2);

        // 3. Reinforce local swirling motion.
        calculate_vorticity(u1, v1, vorticity);
        apply_vorticity_confinement(u1, v1, vorticity, dt);
        velocity_boundaries(u1, v1);

        // 4. Measure how much the velocity field expands/compresses.
        calculate_divergence(u1, v1, divergence);
#if PRINT_DIVERGENCE
        float before = average_divergence(divergence);
#endif

        // 5. Iteratively solve pressure, then subtract its gradient.
        for(int k = 0; k < PRESSURE_ITERATIONS; k++) {
            pressure_iteration(p1, p2, divergence, dt);
            pressure_boundaries(p2);
            swap_pointers(&p1, &p2);
        }

        project_velocity(u1, v1, u2, v2, p1, dt);
        swap_pointers(&u1, &u2);
        swap_pointers(&v1, &v2);

#if PRINT_DIVERGENCE
        calculate_divergence(u1, v1, divergence);
        float after = average_divergence(divergence);
        printf("Divergence: %.6f -> %.6f\n", before, after);
#endif

        // 6. Carry dye through the corrected velocity field and fade it.
        advect_dye(d1, d2, u1, v1, dt);
        swap_pointers(&d1, &d2);

        // 7. Convert dye to grayscale pixels and display the texture.
        update_framebuffer(d1, frameBuffer);
        SDL_UpdateTexture(texture, NULL, frameBuffer, WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // Limit rendering to approximately FPS. The physics step stays fixed.
        uint64_t end = SDL_GetPerformanceCounter();
        double elapsed = (double)(end - start) / (double)SDL_GetPerformanceFrequency();

        if(elapsed < targetFrameTime) {
            SDL_Delay((uint32_t)((targetFrameTime - elapsed) * 1000.0));
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
