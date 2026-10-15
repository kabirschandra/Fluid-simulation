#ifndef CONFIG_H
#define CONFIG_H

// Grid size and display scale. One simulation cell is STRETCH screen pixels.
#define WIDTH 300
#define HEIGHT 200
#define STRETCH 4

// Fixed simulation step. FPS also sets the target display rate.
#define FPS 60.0f
#define TIMESTEP_SCALE 1.0f

// Simulation settings from the latest working version.
#define VISCOSITY 0.0f
#define VORTICITY_STRENGTH 5.0f
#define PRESSURE_ITERATIONS 10
#define DYE_DISSIPATION 0.999f

// Mouse input settings, measured in simulation grid cells.
#define MOUSE_RADIUS 10.0f
#define MOUSE_STRENGTH 10.0f

// Set to 1 to print average absolute divergence before/after projection.
#define PRINT_DIVERGENCE 0

#endif
