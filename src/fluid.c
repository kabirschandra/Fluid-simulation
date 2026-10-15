#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define WIDTH 300
#define HEIGHT 200
#define TIMESTEP_SCALE 1.0f
#define FPS 60.0f
#define VISCOSITY 2.0f
#define STRETCH 4
#define MOUSE_RADIUS 10.0f
#define MOUSE_STRENGTH 1.0f

static float uA[WIDTH * HEIGHT];
static float vA[WIDTH * HEIGHT];
static float uB[WIDTH * HEIGHT];
static float vB[WIDTH * HEIGHT];
static float dyeA[WIDTH * HEIGHT];
static float dyeB[WIDTH * HEIGHT];
static float divergence[WIDTH * HEIGHT];
static float pressureA[WIDTH * HEIGHT];
static float pressureB[WIDTH * HEIGHT];
static uint32_t frameBuffer[WIDTH * HEIGHT];


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


void pressure_iteration(float *oldP, float *newP, float *divergence, float dt) {

    for(int y = 1; y < HEIGHT - 1; y++) {
        for(int x = 1; x < WIDTH - 1; x++) {

            int i = y * WIDTH + x;
            
            float l = oldP[i - 1];
            float r = oldP[i + 1];
            float t = oldP[i - WIDTH];
            float b = oldP[i + WIDTH];

            newP[i] = (l + r + t + b - ((divergence[i]) / dt)) / 4.0f;
        }
    }

    return;
}


float velocity_magnitude(int x, int y) {

    float xU = uA[y * WIDTH + x];
    float yV = vA[y * WIDTH + x];

    float m = sqrtf(xU * xU + yV * yV);

    return m;
}


float lerp(float a, float b, float t) {

    return ((1-t)*a) + (t*b);
}


float bilinear(float A, float B, float C, float D, float fx, float fy) {

    float l1 = lerp(A, B, fx);
    float l2 = lerp(C, D, fx);
    float l3 = lerp(l1, l2, fy);

    return l3;
}


float clampf(float value, float min, float max) {

    return fminf(fmaxf(value, min), max);
}

int maxi(int a, int b) {
    return (a > b) ? a : b;
}
int mini(int a, int b) {
    return (a < b) ? a : b;
}

int clampi(int value, int min, int max) {

    return mini(maxi(value, min), max);
}



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

void advect_dye(float *oldDye, float *newDye, float *u, float *v, float dt) {
    for(int y = 0; y < HEIGHT; y++) {

        for(int x = 0; x < WIDTH; x++) {
            int i = y * WIDTH + x;

            float sourceX = x - u[i] * dt;
            float sourceY = y - v[i] * dt;

            newDye[i] = sample(oldDye, sourceX, sourceY);
        }
    }
    return;
}

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


void init_velocity(void) {

    float cx = WIDTH / 2.0f;
    float cy = HEIGHT / 2.0f;
    float omega = 0.5f;

    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {

            uA[y*WIDTH + x] = 60.0f;
            vA[y*WIDTH + x] = 60.0f;
            //uA[y * WIDTH + x] = - omega * (y - cy);
            //vA[y * WIDTH + x] = omega * (x - cx);
        }
    }

    return;
}


uint8_t dye_intensity(float concentration) {

    concentration = clampf(concentration, 0.0f, 1.0f);

    return (uint8_t)(255 * concentration);
}

void diffuse_velocity(float *oldU, float *oldV, float *newU, float *newV, float viscosity, float dt) {

    // Top and bottom boundaries
    for (int x = 0; x < WIDTH; x++) {
        int top = x;
        int bottom = (HEIGHT - 1) * WIDTH + x;

        newU[top] = 0.0f;
        newV[top] = 0.0f;

        newU[bottom] = 0.0f;
        newV[bottom] = 0.0f;
    }

    // Left and right boundaries
    for (int y = 0; y < HEIGHT; y++) {
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


void update_framebuffer(float *dye) {
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        uint8_t intensity = dye_intensity(dye[i]);

        frameBuffer[i] = (intensity << 16) | (intensity << 8) | intensity;
    }
}


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



void swap_pointers(float **p1, float **p2) {

    float *temp = *p1;

    *p1 = *p2;
    *p2 = temp;

    return;
}


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




int main(void) {

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL Init failure: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Fluid Simulation",
        WIDTH * STRETCH,
        HEIGHT * STRETCH,
        0
    );

    if (!window) {
        fprintf(stderr, "SDL Window failure: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);

    if (!renderer) {
        fprintf(stderr, "SDL Renderer failure: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_XRGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        WIDTH,
        HEIGHT
    );

    if (!texture) {
        fprintf(stderr, "SDL Texture failure: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);



    //init_velocity();
    //init_dye(dyeA);

    float *d1 = dyeA;
    float *d2 = dyeB;

    float *u1 = uA;
    float *u2 = uB;

    float *v1 = vA;
    float *v2 = vB;

    float *p1 = pressureA;
    float *p2 = pressureB;

    float dt = TIMESTEP_SCALE * (1/FPS);

    bool running = true;
    const double targetFrameTime = 1.0 / FPS;

    while (running) {



        uint64_t start = SDL_GetPerformanceCounter();


        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }

            if (event.type == SDL_EVENT_MOUSE_MOTION) {

                if (event.motion.state & SDL_BUTTON_LMASK) {

                    float gridX = event.motion.x / STRETCH;
                    float gridY = event.motion.y / STRETCH;

                    float gridDX = event.motion.xrel / STRETCH;
                    float gridDY = event.motion.yrel / STRETCH;

                    inject_velocity(u1, v1, gridX, gridY, gridDX, gridDY);

                    int x = clampi((int)gridX, 0, WIDTH - 1);
                    int y = clampi((int)gridY, 0, HEIGHT - 1);
                    int i = y * WIDTH + x;

                    //printf("Mouse dx=%.2f dy=%.2f | u=%.2f v=%.2f\n", gridDX, gridDY, u1[i], v1[i]);

                    inject_dye(d1, gridX, gridY);
                }
            }
        }

        // 1. Advect velocity
        advect_velocity(u1, v1, u2, v2, dt);
        swap_pointers(&u1, &u2);
        swap_pointers(&v1, &v2);

        // 2. Diffuse velocity
        diffuse_velocity(u1, v1, u2, v2, VISCOSITY, dt);
        swap_pointers(&u1, &u2);
        swap_pointers(&v1, &v2);

        // 3. Advect dye
        advect_dye(d1, d2, u1, v1, dt);
        update_framebuffer(d2);
        swap_pointers(&d1, &d2);



        SDL_UpdateTexture(
            texture,
            NULL,
            frameBuffer,
            WIDTH * sizeof(uint32_t)
        );

        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);


        uint64_t end = SDL_GetPerformanceCounter();

        double elapsed =
            (double)(end - start) /
            (double)SDL_GetPerformanceFrequency();

        if (elapsed < targetFrameTime) {
            SDL_Delay((uint32_t)((targetFrameTime - elapsed) * 1000.0));
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}


