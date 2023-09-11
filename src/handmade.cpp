#include "handmade.h"
#include <stdint.h>

internal void game_output_sound(game_sound_output_buffer *sound_buffer, int tone_hz) {
    uint16_t tone_volume = 1000;
    local_persist real32 t_sine;
    int wave_samples_pre_period = sound_buffer->samples_per_second/tone_hz;

    int16_t *sample_out = sound_buffer->samples;

    for (DWORD sample_index = 0; sample_index < sound_buffer->sample_count_to_output; ++sample_index) {
        real32 sine_value = sinf(t_sine);
        int16_t sample_value = (int16_t)(sine_value * tone_volume);
        
        *sample_out++ = sample_value; // LEFT
        *sample_out++ = sample_value; // RIGHT
        
        t_sine += 2.0f*FLOAT_PI*1.0f / wave_samples_pre_period;
    }
}

internal void render_wierd_gradient(game_offscreen_buffer *buffer, int x_offset, int y_offset) {
    // doing this makes the pointer math easier as it prevents c from multiplying 
    // the values with the size of variable
    uint8_t *row = (uint8_t*) buffer->memory;
    for (int y=0; y < buffer->height; ++y) {
        // uint32_t *pixel = (uint32_t*) row;
        uint32_t *pixel = (uint32_t*) row;
        
        for (int x=0; x < buffer->width; ++x) {
            /*
                                  0  1  2  3
                Pixel in memory: 00 00 00 00
                                 BB GG RR xx
                last byte is just empty as this processors are designed to read
                4 byte windows faster
                LITTLE ENDIAN ARCHITECTURE: lower byte is placed first

                0x xxBBGGRR --> 
            */

           uint8_t blue = x - x_offset;
           uint8_t green = y - y_offset;
           uint8_t red = 0;
            
            *pixel++ = ((red << 16) | (green << 8) | blue);
        }

        row += buffer->pitch;
    } 
}

internal void game_update_and_render(game_offscreen_buffer *buffer, 
    int x_offset, int y_offset, game_sound_output_buffer *sound_buffer,
    int tone_hz) {
    // TODO: Allow sample offsets here for more robust platform option
    game_output_sound(sound_buffer, tone_hz);

    render_wierd_gradient(buffer, x_offset, y_offset);
}
