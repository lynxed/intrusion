#include <stdbool.h>
#include <SDL.h>
#include <GL/gl.h>
#include "defines.h"
#include "clib.h"
#include "sdl_functions.h"
#include "gl_functions.h"
#include "vector.h"
#include "shader.h"
#include "sound.h"
#include "keypoint.h"
#include "shader_code.h"

static struct scene_state scene_state;
static const struct keypoint* keypoint = keypoints;

static stdcall void initialize_sdl()
{
    sdl.SDL_SetVideoMode(RESOLUTION_X, RESOLUTION_Y, 0,
            SDL_OPENGL | (FULLSCREEN ? SDL_FULLSCREEN : 0));
    sdl.SDL_ShowCursor(SDL_DISABLE);
}

static stdcall void cleanup_sdl()
{
    // SDL_Quit crashes since main() is removed, but we need this call to reset
    // the screen resolution when running fullscreen
    sdl.SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

static stdcall void setup_viewport()
{
    gl.glMatrixMode(GL_PROJECTION);
    gl.glOrtho(-WINDOW_RATIO, WINDOW_RATIO, -1.0, 1.0, -1.0, 1.0);
}

static stdcall bool exit_requested()
{
    SDL_Event event;
    sdl.SDL_PollEvent(&event);

    if (event.type == SDL_QUIT)
    {
        return true;
    }

    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
    {
        return true;
    }

    return false;
}

static stdcall bool update_scene()
{
    Uint32 time = sdl.SDL_GetTicks();
    const struct keypoint* next = keypoint + 1;

    if (time > keypoints[sizeof(keypoints) / sizeof(struct keypoint) - 1].time)
    {
        return false;
    }

    if (time > next->time)
    {
        keypoint += 1;
        next += 1;
    }

    const float time_factor =
        (float)(time - keypoint->time) / (next->time - keypoint->time);

    float* state = (float*)&scene_state;
    float* origin = (float*)&keypoint->state;
    float* destination = (float*)&next->state;

    for (int i = 0; i < sizeof(struct scene_state) / sizeof(float); i++)
    {
        state[i] = origin[i] + (destination[i] - origin[i]) * time_factor;
    }

    return true;
}

static stdcall void mainloop()
{
    GLuint program = compile_program(vertex_glsl, fragment_glsl);
    gl.glUseProgram(program);

    play_sound(); // immedialty before entering mainloop to avoid displacements
    while (!exit_requested() && update_scene())
    {
        uniform_vector3(program, "x", scene_state.position);
        uniform_matrix3(program, "o", scene_state.orientation);
        // Since the three parameters follow each other in the struct,
        // we just treat them as vector to save some bytes.
        // It looks the same in memory anyway.
        uniform_vector3(program, "f", (float*)&scene_state.box_scale);

        gl.glBegin(GL_QUADS);
        gl.glVertex3f(-WINDOW_RATIO, -1.0, 0.0);
        gl.glVertex3f( WINDOW_RATIO, -1.0, 0.0);
        gl.glVertex3f( WINDOW_RATIO,  1.0, 0.0);
        gl.glVertex3f(-WINDOW_RATIO,  1.0, 0.0);
        gl.glEnd();

        sdl.SDL_GL_SwapBuffers();
    }
}

// Save overhead from crt1.o, get control from entry point on
void _start()
{
    initialize_sdl_functions();
    initialize_sdl();
    initialize_gl_functions();
    initialize_sound();

    setup_viewport();

    mainloop();

    cleanup_sdl();

    exit_(0);
}

