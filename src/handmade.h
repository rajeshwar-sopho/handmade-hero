#include <stdint.h>


#define internal static
#define local_persist static
#define global_variable static

typedef uint32_t bool_32;
typedef float real32;
typedef double real64;

#define BYTES_PER_PIXEL 4
#define FLOAT_PI 3.14159265358979323846f

/*
    NOTE: Services that the game provides to the platform
*/

// THIS TAKES 4 THINGS
// Timing, controller/keyboard input, bitmap buffer to use, sound buffer to use

struct game_offscreen_buffer {
    void *memory;
    int width;
    int height;
    int pitch;
};

struct game_sound_output_buffer {
    int sample_count_to_output;
    int16_t *samples;
    int samples_per_second;
    int wave_samples_pre_period;
};

/*
    NOTE: Services that the paltform layer provides to the game
*/

internal void game_update_and_render(
    game_offscreen_buffer *buffer, 
    int x_offset, 
    int y_offset,
    game_sound_output_buffer *sound_buffer,
    int tone_hz
);
