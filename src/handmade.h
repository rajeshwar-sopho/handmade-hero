
#define internal static
#define local_persist static
#define global_variable static

typedef uint32_t bool_32;
typedef float real32;
typedef double real64;

/*
    NOTE: Services that the game provides to the platform
*/

// THIS TAKES 4 THINGS
// Timing, controller/keyboard input, bitmap buffer to use, sound buffer to use
internal void game_update_and_render();

struct game_offscreen_buffer {
    void *memory;
    int width;
    int height;
    int pitch;
};

/*
    NOTE: Services that the paltform layer provides to the game
*/


