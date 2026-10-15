# 2D Fluid Simulation (C / SDL3)

A small interactive 2D fluid simulation written in C. Drag the left mouse button to push the fluid around and add white dye. SDL3 draws the dye as a grayscale texture.

This is the original cell-centred simulation split into separate files. The maths and update order are largely unchanged. It is intended as a visual fluid simulation, not a validated CFD solver.

## Build

Requirements: a C11 compiler, SDL3 development headers/library, `pkg-config`, and `make`.

On macOS with Homebrew:

```sh
brew install sdl3 pkg-config
make
./fluid_simulation
```

On Linux, install SDL3 development files, `pkg-config`, a compiler, and `make` through your distribution. Then run the same `make` and `./fluid_simulation` commands.

On Windows, use an environment with SDL3 and `pkg-config` available (for example MSYS2), then build with `make`. You can also compile `src/*.c` in an IDE configured for SDL3. The Makefile expects a Unix-like shell.

## Controls

Hold **left mouse button** and drag: add momentum and dye. Close the window to quit.

## File layout

| File | Purpose |
| --- | --- |
| `src/main.c` | SDL setup, event loop, simulation update order, and rendering calls |
| `src/config.h` | Grid size, frame rate, viscosity, mouse settings, and solver parameters |
| `src/grid.c/.h` | Clamping, interpolation, grid sampling, pointer swaps |
| `src/fluid.c/.h` | Velocity advection/diffusion, curl, boundaries, divergence, pressure, projection |
| `src/dye.c/.h` | Dye injection, advection, fading, and test pattern |
| `src/input.c/.h` | Mouse momentum brush |
| `src/render.c/.h` | Dye concentration to grayscale pixels |
| `Makefile` | Build command |

The `.h` files declare functions; the `.c` files implement them. `main.c` calls those functions in the correct order.

## How the simulation works

### 1. The grid

The fluid is a `WIDTH x HEIGHT` grid. Each cell stores horizontal velocity `u`, vertical velocity `v`, and dye concentration. Pressure, divergence, and vorticity are extra arrays used during the update. All arrays are flattened: `i = y * WIDTH + x`.

`u` and `v` are in grid cells per second. `dt` is the time step in seconds. `STRETCH` only affects the displayed pixel size and conversion from mouse pixels to grid cells.

### 2. Mouse input

Mouse movement adds velocity in the direction of the drag. A circular brush applies a stronger push near the cursor and a weaker push at its edge. It also deposits dye, capped at `1.0` per cell. The input is added directly to the current buffers before the next simulation update.

### 3. Velocity advection

Advection means moving something along with the fluid. For each grid cell `(x,y)`, we trace backward through the velocity field:

`sourceX = x - u[i] * dt`

`sourceY = y - v[i] * dt`

We then sample the old velocity at that source position. `sample()` uses bilinear interpolation because the source usually lies between grid cells. This is **semi-Lagrangian advection**: it is fairly stable, but it also smooths small details over time.

### 4. Velocity diffusion

Viscosity spreads momentum to nearby cells. The code approximates the Laplacian with the four neighbouring cells:

`laplacianU = left + right + top + bottom - 4 * centre`

`newU = oldU + viscosity * dt * laplacianU`

The same calculation applies to `v`. `VISCOSITY` is currently `0.0`, so this step mostly copies velocity and enforces wall values. If viscosity is increased, the explicit diffusion scheme needs a sufficiently small `viscosity * dt` (at grid spacing 1, normally at most 0.25 for the diffusion step alone).

### 5. Vorticity confinement

Vorticity measures local rotation:

`omega = dv/dx - du/dy`

The simulation calculates how the **magnitude** of vorticity changes across the grid. It uses that direction and the sign of `omega` to add a rotational force. This helps retain swirls that semi-Lagrangian advection would otherwise smooth out. It is an artificial visual effect; a large `VORTICITY_STRENGTH` can inject too much energy.

### 6. Divergence and pressure

Divergence measures local expansion or compression:

`divergence = du/dx + dv/dy`

For an incompressible fluid we want this close to zero. The pressure solver repeatedly updates the pressure grid using Jacobi iterations, then projection subtracts the pressure gradient:

`u_new = u_old - dt * dp/dx`

`v_new = v_old - dt * dp/dy`

This pushes the velocity field toward lower divergence. `PRESSURE_ITERATIONS` controls the number of Jacobi passes. The arrays are swapped after each pass rather than copied.

**Numerical limitation:** The current pressure iteration uses neighbours two cells away, while pressure wall conditions only fill the outermost cells. The first interior layer is not solved by that iteration, and the two-cell stencil can split into odd/even pressure modes. This means the pressure solve is only approximate, especially near walls. Divergence may decrease without reaching zero. This should be revisited before using the code for physically accurate results. `PRINT_DIVERGENCE=1` prints the before/after values to help test it.

### 7. Dye advection and dissipation

Dye uses the corrected velocity field to trace backward, just like velocity advection. Each sampled concentration is multiplied by `DYE_DISSIPATION`. Values slightly below `1` make the dye fade gradually. This is a visual fade, not a physically modelled chemical process.

### 8. Rendering

The dye concentration is clamped to `[0,1]`, converted to an 8-bit intensity, and copied into the red, green, and blue channels of an XRGB8888 pixel. SDL3 uploads this framebuffer to a texture and draws it scaled by `STRETCH`.

## Why there are two buffers

Functions like `advect_velocity()` must read the old grid without changing it partway through the calculation. They write to a second grid instead. `swap_pointers()` then switches which buffer is current. There are two buffers for each velocity component, dye, and pressure.

## Update order

1. Handle mouse input and inject momentum/dye.
2. Advect velocity.
3. Diffuse velocity.
4. Calculate vorticity and apply confinement.
5. Apply velocity boundaries.
6. Calculate divergence.
7. Run pressure Jacobi iterations.
8. Subtract the pressure gradient and enforce velocity walls.
9. Advect dye, apply dissipation.
10. Convert dye to pixels and render.

The order matters: dye should follow the velocity field **after** the pressure correction.

## Settings to change

All settings are in `src/config.h`:

- `WIDTH`, `HEIGHT`: number of simulation cells. Larger grids cost more CPU time.
- `STRETCH`: how many screen pixels each cell occupies.
- `FPS`, `TIMESTEP_SCALE`: frame target and fixed physics step.
- `VISCOSITY`: how strongly momentum spreads.
- `VORTICITY_STRENGTH`: strength of artificial swirl reinforcement.
- `PRESSURE_ITERATIONS`: number of Jacobi pressure passes.
- `DYE_DISSIPATION`: dye retention per simulation frame (`1.0` means no fade).
- `MOUSE_RADIUS`, `MOUSE_STRENGTH`: brush size and injected momentum.
- `PRINT_DIVERGENCE`: turn console diagnostics on or off.

Current settings match the last uploaded source: vorticity strength `100`, pressure iterations `10`, dye dissipation `0.999`. If the simulation becomes unstable, try reducing vorticity strength first.

## Test patterns

`main.c` contains two commented-out initialisation calls. Uncomment `init_velocity(uA, vA)` to fill the grid with a uniform test velocity, or `init_dye(dyeA)` to add a small square of dye. These are useful when checking advection without moving the mouse. The boundary conditions will still stop velocity at the walls.

## Things to improve later

- Make the pressure solver and wall treatment fully consistent, or move to a staggered grid if numerical accuracy becomes important.
- Use a real-time accumulator if the simulation needs to keep correct elapsed time when the frame rate drops. Currently `dt` stays fixed even if rendering is slow.
- Separate dye into RGB channels for coloured smoke.
- Improve mouse input so the injected velocity depends less on event frequency.
- Profile the pressure solve and grid sampling before increasing resolution.

