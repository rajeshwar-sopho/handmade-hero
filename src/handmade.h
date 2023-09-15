#include <stdint.h>
#include <math.h>


#define internal static
#define local_persist static
#define global_variable static

typedef uint32_t bool_32;
typedef float real32;
typedef double real64;

#define BYTES_PER_PIXEL 4
#define FLOAT_PI 3.14159265358979323846f

#define array_len(array) (sizeof(array) / sizeof((array)[0]))

#define kilobytes(value) ((value) * 1024)
#define megabytes(value) (kilobytes(value) * 1024)
#define gigabytes(value) (megabytes(value) * 1024)
#define terabyte(value)  (gigabytes(value) * 1024)

// this just means write into the null pointer thus crashing the program
#if HANDMADE_SLOW
#define assert(expression) if(!(expression)) {*(int*)0=0;}
#else
#define assert(expression)
#endif


#if HANDMADE_INTERNAL

struct debug_read_file_result {
  uint32_t contents_size;
  void *contents;
};

internal void DEBUGplatform_free_file_memory(void *memory);
internal debug_read_file_result DEBUGplatform_read_entire_file(char *filename);

internal bool_32 DEBUGplatform_write_entire_file(char *filename, uint32_t memory_size, void *memory);
#endif

inline uint32_t safe_truncate_uint64(uint64_t value) {
  assert(value <= 0xFFFFFFFF);
  uint32_t result = (uint32_t)value;
  return result;
}

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
};

struct win32_sound_output {
    int samples_per_second;
    uint32_t running_sample_index;
    int bytes_per_sample;
    int secondary_buffer_size;
    int latency_sample_count;
};


struct game_button_state {
    int half_transition_count;
    bool_32 ended_down;
};

struct game_controller_input {
    bool_32 is_analog;

    real32 start_x;
    real32 start_y;

    real32 min_x;
    real32 min_y;

    real32 max_x;
    real32 max_y;

    real32 end_x;
    real32 end_y;

    union {
        game_button_state buttons[6];
        struct {
            game_button_state up;
            game_button_state down;
            game_button_state left;
            game_button_state right;
            game_button_state left_shoulder;
            game_button_state right_shoulder;
        };
    };
};

struct game_input {
    game_controller_input controllers[4];
};

struct game_memory {
    bool_32 is_initialised;
    
    uint64_t permanent_memory_size;
    void *permanent_storage;

    uint64_t transient_memory_size;
    void *transient_storage;  
};

struct game_state {
    int x_offset;
    int y_offset;
    int tone_hz;
};

internal void game_update_and_render(
    game_memory *memory,
    game_input *digital_input,
    game_offscreen_buffer *buffer, 
    game_sound_output_buffer *sound_buffer
);
