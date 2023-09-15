#include "handmade.h"
#include <stdint.h>

internal void game_output_sound(
    game_sound_output_buffer *sound_buffer, 
    int tone_hz) 
{
    uint16_t tone_volume = 1000;
    local_persist real32 t_sine;
    int wave_samples_pre_period = sound_buffer->samples_per_second/tone_hz;

    int16_t *sample_out = sound_buffer->samples;

    for (uint32_t sample_index = 0; sample_index < sound_buffer->sample_count_to_output; ++sample_index) {
        real32 sine_value = sinf(t_sine);
        int16_t sample_value = (int16_t)(sine_value * tone_volume);
        
        *sample_out++ = sample_value; // LEFT
        *sample_out++ = sample_value; // RIGHT
        
        t_sine += 2.0f*FLOAT_PI*1.0f / wave_samples_pre_period;
    }
}

internal void render_wierd_gradient(
    game_offscreen_buffer *buffer, 
    int x_offset, 
    int y_offset) 
{
    uint8_t *row = (uint8_t*) buffer->memory;
    for (int y=0; y < buffer->height; ++y) {
        uint32_t *pixel = (uint32_t*) row;
        for (int x=0; x < buffer->width; ++x) {
            uint8_t blue = x - x_offset;
            uint8_t green = y - y_offset;
            uint8_t red = 0;
            *pixel++ = ((red << 16) | (green << 8) | blue);
        }
        row += buffer->pitch;
    } 
}

internal void game_update_and_render(
    game_memory *memory,
    game_input *digital_input,
    game_offscreen_buffer *buffer,
    game_sound_output_buffer *sound_buffer) 
{
   assert(sizeof(game_state) <= memory->permanent_memory_size);
     
    game_state *current_state = (game_state*)memory->permanent_storage;

    if (!memory->is_initialised) {
      
      char *filename = __FILE__;
      char *write_filename = "C:\\Users\\rajes\\somefile.txt";
      debug_read_file_result bitmap = DEBUGplatform_read_entire_file(filename);
      if (bitmap.contents) {
        DEBUGplatform_write_entire_file(write_filename, bitmap.contents_size, bitmap.contents);
        DEBUGplatform_free_file_memory(bitmap.contents);
      }

      current_state->x_offset = 0;
      current_state->y_offset = 0;
      current_state->tone_hz = 440;

      memory->is_initialised = true;
    }


    game_controller_input player1_input = digital_input->controllers[0];
    if (player1_input.is_analog) {
        current_state->tone_hz += (int)(128.0f*player1_input.end_y);
        current_state->x_offset += (int)(4.0f*player1_input.end_y);
    } else {

    }

    if (player1_input.down.ended_down) {
        current_state->y_offset += 1;
    }

    // TODO: Allow sample offsets here for more robust platform option
    game_output_sound(sound_buffer, current_state->tone_hz);
    render_wierd_gradient(buffer, current_state->x_offset, current_state->y_offset);
}
