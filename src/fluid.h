#ifndef FLUID_H
#define FLUID_H

void velocity_boundaries(float *u, float *v);
void project_velocity(float *oldU, float *oldV, float *newU, float *newV, float *p, float dt);
void pressure_boundaries(float *p);
void calculate_divergence(float *u, float *v, float *divergence);
void pressure_iteration(float *oldP, float *newP, float *divergence, float dt);
void advect_velocity(float *oldU, float *oldV, float *newU, float *newV, float dt);
void calculate_vorticity(float *u, float *v, float *vorticity);
void diffuse_velocity(float *oldU, float *oldV, float *newU, float *newV, float viscosity, float dt);
float average_divergence(float *divergence);
void apply_vorticity_confinement(float *u, float *v, float *vorticity, float dt);
void init_velocity(float *u, float *v);

#endif
