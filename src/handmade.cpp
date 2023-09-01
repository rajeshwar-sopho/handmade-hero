#include "handmade.h"
#include <stdint.h>

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

internal void game_update_and_render(game_offscreen_buffer *buffer, int x_offset, int y_offset) {
    render_wierd_gradient(buffer, x_offset, y_offset);
}
