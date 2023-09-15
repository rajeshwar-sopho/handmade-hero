/*
    Planned stuff that should be done
    [(
    TODO: This is not the final platform layer.

    - Saved games location
    - Getting a handle to our own exec file
    - assets loading path
    - Threading (launching threads)
    - Raw Input (keyboard and mouse)
    - Sleep/timeBeginPeriod
    - ClipCursor for multimonitor support
    - Full Screen suppoprt
    - WM_SETCURSOR for cursor visibility
    - QueryCancleAutoplay
    - WM_ACTIVEAPP (when we are not the active app)
    - Blit speed improvements (BitBlit)
    - Hardware acceleration (OpenGL or Direct3D or both)
    - Get keyboard layout for french keyboards, international WASD support
    )]

*/

// header stuff for now

#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <Xinput.h>
#include <DSound.h>

// implement sinf ourself
#include <math.h>
#include <winnt.h>
#include "handmade.cpp"
// https://github.com/Renardjojo/PetForDesktop: pet for desktop (build something similar)

#define internal static
#define local_persist static
#define global_variable static

#define BYTES_PER_PIXEL 4
#define FLOAT_PI 3.14159265358979323846f

// typedef uint8_t uint8;

typedef uint32_t bool_32; 
typedef double real64;

struct win32_offscreen_buffer {
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
    int memory_size;
};

// TODO: global variable for now
global_variable bool_32 global_running;
global_variable win32_offscreen_buffer global_back_buffer;
global_variable LPDIRECTSOUNDBUFFER global_secondary_buffer;


// loading xinput and direct sound library
// NOTE: Support for XInputSetState
// if we do it like this then if want to change the function signature then we can just change in one place
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)

// we are making this in order to create a function pointer
typedef X_INPUT_SET_STATE(x_input_set_state);

// this is a stub function that we can use to initialize our function pointer
X_INPUT_SET_STATE(x_input_set_state_stub) {
    return ERROR_DEVICE_NOT_CONNECTED;
}

// this is the function pointer
global_variable x_input_set_state *XInputSetState_ = x_input_set_state_stub;

// we are setting function pointer to XInputSetState to avoid errors when its not supported 
#define XInputSetState XInputSetState_

// NOTE: Support for XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(x_input_get_state_stub) {
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = x_input_get_state_stub;
#define XInputGetState XInputGetState_


#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);


internal void win32_loadxinput(void) {
    HMODULE x_input_lib = LoadLibraryA("XInput1_4.dll");

    if (!x_input_lib) {
        x_input_lib = LoadLibraryA("xinput9_1_0.dll");
    }

    if (!x_input_lib) {
        x_input_lib = LoadLibraryA("XInput1_3.dll");
    }

    if (x_input_lib) {
        XInputGetState = (x_input_get_state*)GetProcAddress(x_input_lib, "XInputGetState");
        XInputSetState = (x_input_set_state*)GetProcAddress(x_input_lib, "XInputSetState");
    }
}

struct win32_window_dimension {
    int height;
    int width;
};


internal void DEBUGplatform_free_file_memory(void *memory)
{
  if (memory) {
    VirtualFree(memory, 0, MEM_RELEASE);
  }
}

internal debug_read_file_result DEBUGplatform_read_entire_file(char *filename)
{

  debug_read_file_result result = {}; 
  HANDLE file_handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);


  if (file_handle != INVALID_HANDLE_VALUE) {
    LARGE_INTEGER file_size;
    if (GetFileSizeEx(file_handle, &file_size)) {
      uint32_t file_size32 = safe_truncate_uint64(file_size.QuadPart);
      result.contents = VirtualAlloc(0, file_size32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
      if (result.contents) {
        DWORD bytes_read;
        if (ReadFile(file_handle, result.contents, file_size32, 
              &bytes_read, 0) && (file_size32 == bytes_read)) {
          result.contents_size = file_size32; 
        } else {
          DEBUGplatform_free_file_memory(result.contents);
          result = {};
        }
      } else {
        // TODO: logging 
      }
    } else {
      // TODO: logging
    }

    CloseHandle(file_handle);
  } else {
      // TODO: logging
  }


  // BOOL CloseHandle(
  //   hObject
  // );

  return result;
}

internal bool_32 DEBUGplatform_write_entire_file(char *filename, uint32_t memory_size, void *memory)
{

  bool_32 result = false; 
  HANDLE file_handle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);


  if (file_handle != INVALID_HANDLE_VALUE) {
    DWORD bytes_writer;
    if (WriteFile(file_handle, memory, memory_size, 
          &bytes_writer, 0)) {
         result = (bytes_writer == memory_size);
    } else {
      // TODO: logging
    }

    CloseHandle(file_handle);
  } else {
      // TODO: logging
  }
  return result;
}

internal win32_window_dimension win32_get_window_dim(HWND window) {
    RECT client_rect;
    GetClientRect(window, &client_rect);
    win32_window_dimension dimension;

    dimension.height = client_rect.bottom - client_rect.top;
    dimension.width = client_rect.right - client_rect.left;

    return dimension;
} 


// this function is called only when window is getting resized
// in this function we are changing valuse of the buffer itself therefore passing by reference
internal void win32_resize_DIB_section(win32_offscreen_buffer *buffer, int window_width, int window_height) {
    // TODO: Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails

    if (buffer->memory) {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->height = window_height;
    buffer->width = window_width;

    buffer->pitch = buffer->width * BYTES_PER_PIXEL;
    // this formula and details about the bitmap header are mentioned on below page
    // https://learn.microsoft.com/en-us/windows/win32/gdi/device-independent-bitmaps
    buffer->memory_size = BYTES_PER_PIXEL * buffer->width * buffer->height;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height; // this will make windows draw from top to down se the biheight on msdn
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;
    
    // virtual alloc is faster as malloc will go through a bunch of code and eventually call
    // virtualalloc, as eventually we need memory in the form of pages where a page is just a byte
    // windows allocates virtual memory to the program not actual memory for safety
    buffer->memory = VirtualAlloc(0, buffer->memory_size, MEM_COMMIT, PAGE_READWRITE);
}


// it's probably better to pass small structs or variables just by reference as compiler will make
// them inline to optimize them we can replace RECT *client_rect to RECT client_rect

// this function will get called frequently
internal void win32_update_window(win32_offscreen_buffer *buffer, HDC device_context, int window_width, int window_height) {

    // TODO: Aspect ratio fix

    // its going to copy bitmap memory object to the window and its in rgb colors
    // since bitmap_height and width are global and every time we resize the window
    // we are setting them equal to the h and w of the window, they are same
    StretchDIBits( device_context,
        /*
        x, y, width, height,
        x, y, width, height,
        */
        0, 0, window_width, window_height, // which is pointed by the device_context
        0, 0, buffer->width, buffer->height, // these are the dimensions for memory where bitmap data is stored
        buffer->memory,
        &buffer->info,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

LRESULT CALLBACK mainwindow_callback(HWND window, UINT Msg, WPARAM wParam, LPARAM lParam) {
    // this function is a call back that processes messages received by the window
    LRESULT result = 0;
    switch(Msg) {
        case WM_SIZE:
        {
        } break;
        case WM_DESTROY:
        {
            global_running = false;
            OutputDebugStringA("WM_DESTROY");
        } break;
        case WM_CLOSE:
        {
            global_running = false;
            OutputDebugStringA("WM_CLOSE");
        } break;
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP");
        } break;
        case WM_SYSKEYUP:
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {   
            uint32_t vkcode = wParam;
            bool_32 was_down = (lParam & (1 << 30)) != 0;
            bool_32 is_down = (lParam & (1 << 31)) == 0;

            if (was_down != is_down) {
                if (vkcode == 'W') {

                } else if (vkcode == 'A') {

                } else if (vkcode == 'S') {
                    
                } else if (vkcode == 'W') {
                    
                } else if (vkcode == 'D') {
                    
                } else if (vkcode == VK_UP) {
                    
                } else if (vkcode == VK_DOWN) {
                    
                } else if (vkcode == VK_LEFT) {
                    
                } else if (vkcode == VK_RIGHT) {
                    
                } else if (vkcode == 'Q') {
                    
                } else if (vkcode == 'E') {
                    
                } else if (vkcode == 'R') {
                    
                } else if (vkcode == 'C') {
                    
                } else if (vkcode == 'V') {
                    
                } else if (vkcode == VK_LSHIFT) {
                    
                } else if (vkcode == VK_LCONTROL) {
                    
                } else if (vkcode == VK_SPACE) {
                    
                } else if (vkcode == VK_ESCAPE) {
                    
                }

                // this will result in either 0 or 1 as we are doing and of bits 
                // so we dont care about telling the compiler that its bool
                // if we keep this then compiler will shove this in either 0 or 1
                bool_32 alt_key_was_down = lParam & (1 << 29);
                if (vkcode == VK_F4 && alt_key_was_down) {
                    global_running = false;
                }

            }

        } break;
        case WM_PAINT:
        {
            // so the paint object is giving us both, DC and the rectangle to draw on
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window, &paint);

            win32_window_dimension dimension = win32_get_window_dim(window);

            // render_wierd_gradient(100, 0);
            win32_update_window(&global_back_buffer, device_context, dimension.width, dimension.height);

            EndPaint(window, &paint);
        } break;
        default:
        {
            // OutputDebugStringA("WM_SIZE");
            // for messages that you dont want to process or want window's default behavior call this
            result = DefWindowProc(window, Msg, wParam, lParam) ;
        } break;
    }

    return result;
}


internal void win32_init_dsound(HWND window, int32_t samples_per_second, int32_t buffer_size) {
    // Load the library
    HMODULE dsound_library = LoadLibraryA("dsound.dll");

    if (dsound_library) {
        // get a direct sound object
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(dsound_library, "DirectSoundCreate");
        
        LPDIRECTSOUND direct_sound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &direct_sound, 0))) {
            
            WAVEFORMATEX wave_format = {};
            wave_format.wFormatTag = WAVE_FORMAT_PCM;
            wave_format.nChannels = 2;
            wave_format.wBitsPerSample = 16;
            wave_format.nSamplesPerSec = samples_per_second;
            wave_format.nBlockAlign = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
            wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
            wave_format.cbSize = 0;

            if (SUCCEEDED(direct_sound->SetCooperativeLevel(window, DSSCL_PRIORITY))) {
                // by convention we zero all struct values before setting values
                // below line set the memory size(first in struct) and set rest to 0
                DSBUFFERDESC buffer_description = {};
                buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER;
                buffer_description.dwSize = sizeof(buffer_description);
                buffer_description.dwBufferBytes = 0;


                // create a primary buffer
                LPDIRECTSOUNDBUFFER primary_buffer;
                if (SUCCEEDED(direct_sound->CreateSoundBuffer(&buffer_description, &primary_buffer, 0))) {
                    

                    if (SUCCEEDED(primary_buffer->SetFormat(&wave_format))) {
                        
                    } else {
                        // TODO: diagonastic
                    }
                }
                
            } else {
                // TODO: Diagnostics
            }

            // create a secondary buffer
            DSBUFFERDESC buffer_description = {};
            buffer_description.dwFlags = 0;
            buffer_description.dwSize = sizeof(buffer_description);
            buffer_description.dwBufferBytes = buffer_size;
            buffer_description.lpwfxFormat = &wave_format;

            if (SUCCEEDED(direct_sound->CreateSoundBuffer(&buffer_description, &global_secondary_buffer, 0))) {
                // start it playing 
            }
        }

    } else {
        // TODO: Diagnostics
    }
}

internal void win32_fill_sound_buffer(win32_sound_output *sound_output, DWORD byte_to_lock, DWORD bytes_to_write, game_sound_output_buffer *sound_buffer) {
    // int16 int16
    // [LEFT RIGTH] LEFT RIGTH LEFT RIGTH LEFT RIGTH 
    //
    void *region1;
    DWORD region1_size;
    void *region2;
    DWORD region2_size;


    if (SUCCEEDED(global_secondary_buffer->Lock(byte_to_lock, bytes_to_write,
        &region1, &region1_size,
        &region2, &region2_size,
        0
    ))) {

        int16_t *dest_samples = (int16_t*)region1;
        DWORD region1_sample_count = region1_size/sound_output->bytes_per_sample;
        int16_t *source_samples = sound_buffer->samples;

        for (DWORD sample_index = 0; sample_index < region1_sample_count; ++sample_index) {
            *dest_samples++ = *source_samples++; // LEFT
            *dest_samples++ = *source_samples++; // RIGHT
             ++sound_output->running_sample_index;
        }

        DWORD region2_sample_count = region2_size/sound_output->bytes_per_sample;
        dest_samples = (int16_t*)region2;
        for (DWORD sample_index = 0; sample_index < region2_sample_count; ++sample_index) {
            *dest_samples++ = *source_samples++; // LEFT
            *dest_samples++ = *source_samples++; // RIGHT
             ++sound_output->running_sample_index;
        }
        
        global_secondary_buffer->Unlock(region1, region1_size, region2, region2_size);
    }
}

internal void win32_clear_buffer(win32_sound_output *sound_output) 
{
    void *region1;
    DWORD region1_size;
    void *region2;
    DWORD region2_size;

    if (SUCCEEDED(global_secondary_buffer->Lock(0, sound_output->secondary_buffer_size,
                                                &region1, &region1_size,
                                                &region2, &region2_size,
                                                0)))
    {
        uint8_t *dest_samples = (uint8_t*)region1;
        for (DWORD byte_index = 0; byte_index < region1_size; ++byte_index) {
            *dest_samples++ = 0;
        }

        dest_samples = (uint8_t*)region2;
        for (DWORD byte_index = 0; byte_index < region2_size; ++byte_index) {
            *dest_samples++ = 0;
        }

        global_secondary_buffer->Unlock(region1, region1_size, region2, region2_size);
    }
}


internal void win32_process_xinput_digital_button(
    game_button_state *old_state, game_button_state *new_state,
    DWORD button_bit, DWORD xinput_button_state
)
{
    new_state->ended_down = ((xinput_button_state & button_bit) == button_bit);
    new_state->half_transition_count = (old_state->ended_down != new_state->ended_down) ? 1 : 0;
}

// they stick in the kernel code so we can call functions from there
// requirement and library is mentioned in the msdn page at the bottom

// we ca use getmodulehandle(0) to get hinstance variable that is not proper
// name search for it on msdn
// We should rarely use this because this forces us to keep the same controll flow
// accross all the platforms
// #if HANDMADE_WIN32
int WINAPI WinMain(
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance,
    PSTR lpCmdLine, 
    int nCmdShow)
{
    // this is counts per second made
    LARGE_INTEGER pref_counter_freq_result;
    QueryPerformanceFrequency(&pref_counter_freq_result);

    uint64_t pref_counter_freq = pref_counter_freq_result.QuadPart;


    win32_loadxinput();
    
    // DisplayResourceNAMessageBox();
    WNDCLASSA wc = {};
    win32_resize_DIB_section(&global_back_buffer, 1440, 1080);
    wc.style = CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = mainwindow_callback;
    wc.hInstance = hInstance;
    // wc.hIcon = ;
    wc.lpszClassName = "HandmadeHeroWindowClass";

    
    // usually there is no hard need to release the resources and just closing is fine
    // as windows will clear that automatically for us when someone will close the app
    if (RegisterClassA(&wc)) {
        // create a handle to the window wc
        HWND main_window = CreateWindowExA(
            0,
            wc.lpszClassName,
            "Handmade Hero",
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            hInstance,
            0
        );

        if (main_window) {
            global_running = true;

            win32_sound_output sound_output = {};

            sound_output.samples_per_second = 48000;
            sound_output.running_sample_index = 0;
            sound_output.bytes_per_sample = sizeof(int16_t)*2;
            sound_output.secondary_buffer_size = sound_output.samples_per_second * sound_output.bytes_per_sample;
            sound_output.latency_sample_count = sound_output.samples_per_second / 15;

            win32_init_dsound(main_window, sound_output.samples_per_second, sound_output.secondary_buffer_size);
            win32_clear_buffer(&sound_output);
            global_secondary_buffer->Play(0, 0, DSBPLAY_LOOPING);

            int16_t *samples = (int16_t*)VirtualAlloc(0, sound_output.secondary_buffer_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

#if HANDMADE_INTERNAL
            LPVOID base_address = (LPVOID)terabyte((uint64_t)2);
#else 
            LPVOID base_address = 0;
#endif
            
            game_memory memory = {};
            memory.permanent_memory_size = megabytes(64);
            memory.transient_memory_size = gigabytes((uint64_t)4);
            
            uint64_t total_memory_size = memory.permanent_memory_size + memory.transient_memory_size;
            memory.permanent_storage = VirtualAlloc(base_address, total_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            memory.transient_storage = ((uint8_t*)memory.permanent_storage + memory.permanent_memory_size); 

            game_input game_inputs[2] = {};
            game_input *old_input = &game_inputs[0];
            game_input *new_input = &game_inputs[1];
            
            LARGE_INTEGER last_counter;
            QueryPerformanceCounter(&last_counter);

            uint64_t last_cycle_count = __rdtsc();

            while (global_running) {
                // get message
                MSG message;
                // peek message loop when there is no message it will allow us to run instead of blocking
                while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
                    if (message.message == WM_QUIT) {
                        global_running = false;
                    }

                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }

                // TODO: Should we pool this more frequently
                // So lets say if we are getting input 5 then getting input 30, if that's the case that
                // means there is a gap in our code for taking messages, we can poll more often to eliminate this

                int MAX_CONTROLLER_COUNT = XUSER_MAX_COUNT;
                if (MAX_CONTROLLER_COUNT > array_len(new_input->controllers)) {
                    MAX_CONTROLLER_COUNT = array_len(new_input->controllers);
                }

                // This is for xbox controller and playstation controller won't work with this
                for (
                    DWORD controller_index = 0; 
                    controller_index < MAX_CONTROLLER_COUNT; 
                    controller_index++) 
                {
                    XINPUT_STATE controller_state;
                    if (XInputGetState(controller_index, &controller_state) == ERROR_SUCCESS) {

                        game_controller_input *old_state = &old_input->controllers[controller_index];
                        game_controller_input *new_state = &new_input->controllers[controller_index];

                        // NOTE: This controller is plugged in
                        // TODO: See if ControllerState.dwPacketNumber increments too rapidly
                        XINPUT_GAMEPAD *game_pad = &controller_state.Gamepad;

                        // TODO: Dpad
                        bool_32 up = (game_pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool_32 down = (game_pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool_32 left = (game_pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool_32 right = (game_pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

                        // bool_32 start = (game_pad->wButtons & XINPUT_GAMEPAD_START);
                        // bool_32 back = (game_pad->wButtons & XINPUT_GAMEPAD_BACK);

                        win32_process_xinput_digital_button(&old_state->down, &new_state->down, XINPUT_GAMEPAD_A, game_pad->wButtons);
                        win32_process_xinput_digital_button(&old_state->right, &new_state->right, XINPUT_GAMEPAD_B, game_pad->wButtons);
                        win32_process_xinput_digital_button(&old_state->left, &new_state->left, XINPUT_GAMEPAD_X, game_pad->wButtons);
                        win32_process_xinput_digital_button(&old_state->up, &new_state->up, XINPUT_GAMEPAD_Y, game_pad->wButtons);

                        win32_process_xinput_digital_button(&old_state->left_shoulder, &new_state->left_shoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, game_pad->wButtons);
                        win32_process_xinput_digital_button(&old_state->right_shoulder, &new_state->right_shoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, game_pad->wButtons);

                        new_state->is_analog = true;
                        new_state->start_x = old_state->end_x;
                        new_state->start_y = old_state->end_y;
                        
                        real32 x;
                        if (game_pad->sThumbLX < 0) {
                            x = (real32)game_pad->sThumbLX / 32768.0f;
                        } else {
                            x = (real32)game_pad->sThumbLX / 32767.0f;
                        }
                        new_state->min_x = new_state->max_x = new_state->end_x = x;

                        real32 y;
                        if (game_pad->sThumbLY < 0) {
                            y = (real32)game_pad->sThumbLY / 32768.0f;
                        } else {
                            y = (real32)game_pad->sThumbLY / 32767.0f;
                        }
                        new_state->min_y = new_state->max_y = new_state->end_y = y;

                    }
                }

                // // TODO:  Try to implement this for taking input from the PS5 controller
                // we can use ds-4 windows and hidhide to emulate xbox controller
                // https://ds4-windows.com/get-started/

                // TODO: This still is just a work around try to implement fully featured PS5 controller support (P.S. Write a Driver from scratch)

                // Uncomment these lines for vibration
                // XINPUT_VIBRATION vibration;
                // vibration.wLeftMotorSpeed = 60000;
                // vibration.wRightMotorSpeed = 60000;

                // XInputSetState(0, &vibration);
                DWORD play_cursor;
                DWORD write_cursor;
                DWORD byte_to_lock;
                DWORD bytes_to_write;
                DWORD target_cursor;

                bool_32 sound_is_valid = false;

                if (SUCCEEDED(global_secondary_buffer->GetCurrentPosition(&play_cursor, &write_cursor))) {
                    // direct sound output test
                    byte_to_lock = (sound_output.running_sample_index * sound_output.bytes_per_sample) % sound_output.secondary_buffer_size;
                    bytes_to_write;

                    target_cursor = (play_cursor + 
                        (sound_output.latency_sample_count * sound_output.bytes_per_sample)) %
                        sound_output.secondary_buffer_size;

                    // TODO: We need a more accurate chech than byte_to_lock == target_cursor
                    if (byte_to_lock > target_cursor) {
                        bytes_to_write = sound_output.secondary_buffer_size - byte_to_lock;
                        bytes_to_write += target_cursor;
                    } else {
                        bytes_to_write = target_cursor - byte_to_lock;
                    }
                    
                    sound_is_valid = true;
                }

                game_sound_output_buffer sound_buffer = {};
                sound_buffer.samples_per_second = sound_output.samples_per_second;
                sound_buffer.sample_count_to_output = bytes_to_write / sound_output.bytes_per_sample;
                sound_buffer.samples = samples;

                // this populates the bitmap memory object that we created
                game_offscreen_buffer game_buffer;
                game_buffer.height = global_back_buffer.height;
                game_buffer.width = global_back_buffer.width;
                game_buffer.pitch = global_back_buffer.pitch;
                game_buffer.memory = global_back_buffer.memory;

                game_update_and_render(&memory, new_input, &game_buffer, &sound_buffer);
                
                if (sound_is_valid) {
                    win32_fill_sound_buffer(&sound_output, byte_to_lock, bytes_to_write, &sound_buffer);
                }

                // ++x_offset;

                HDC device_context = GetDC(main_window);
                win32_window_dimension dimension = win32_get_window_dim(main_window);

                // this takes the memory object and populate the client rectangle
                win32_update_window(&global_back_buffer, device_context, dimension.width, dimension.height);
                ReleaseDC(main_window, device_context);

                // calculating cycles
                uint64_t end_cycles_count = __rdtsc();

                // this is how many counts it took
                LARGE_INTEGER end_counter;
                QueryPerformanceCounter(&end_counter);

                int64_t count_elapsed = end_counter.QuadPart - last_counter.QuadPart;
                int32_t frame_per_sec = (int32_t)(pref_counter_freq / count_elapsed);
                int64_t frame_time = ((1000*count_elapsed) / pref_counter_freq);
                int64_t cycles_elapsed = end_cycles_count - last_cycle_count;
                int32_t mega_cycles_per_frame = (int32_t)(cycles_elapsed/(1000 * 1000));

                char buffer[250];
                wsprintfA(buffer, "Frame time %dms, %d FPS Cycles per Frame: %d MHz \n", frame_time, frame_per_sec, mega_cycles_per_frame);
                OutputDebugStringA(buffer);

                last_counter = end_counter;
                last_cycle_count = end_cycles_count;

                game_input *temp = new_input;
                new_input = old_input; // this will anyways gets calculated as new in the next loop
                old_input = temp;
            }
        } else {
            // TODO: logging
        }
    } else {
        // TODO: logging
    }

    return 0;
}
