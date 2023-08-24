#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <Xinput.h>
#include <DSound.h>

// https://github.com/Renardjojo/PetForDesktop: pet for desktop (build something similar)

#define internal static
#define local_persist static
#define global_variable static

#define BYTES_PER_PIXEL 4

// typedef uint8_t uint8;

typedef uint32_t bool_32;

struct win32_offscreen_buffer {
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
    int memory_size;
};

// TODO: global variable for now
global_variable bool global_running;
global_variable win32_offscreen_buffer global_back_buffer;

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
    if (x_input_lib) {
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


internal win32_window_dimension win32_get_window_dim(HWND window) {
    RECT client_rect;
    GetClientRect(window, &client_rect);
    win32_window_dimension dimension;

    dimension.height = client_rect.bottom - client_rect.top;
    dimension.width = client_rect.right - client_rect.left;

    return dimension;
} 


internal void render_wierd_gradient(win32_offscreen_buffer *buffer, int x_offset, int y_offset) {
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
        if (DirectSoundCreate && DirectSoundCreate(0, &direct_sound, 0)) {
            
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

            LPDIRECTSOUNDBUFFER secondary_buffer;
            if (SUCCEEDED(direct_sound->CreateSoundBuffer(&buffer_description, &secondary_buffer, 0))) {
                // start it playing 
            }
        }

    } else {
        // TODO: Diagnostics
    }
}

// they stick in the kernel code so we can call functions from there
// requirement and library is mentioned in the msdn page at the bottom

// we ca use getmodulehandle(0) to get hinstance variable that is not proper
// name search for it on msdn
int WINAPI WinMain(
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance,
    PSTR lpCmdLine, 
    int nCmdShow)
{
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
            int x_offset, y_offset;
            x_offset = y_offset = 0;

            win32_init_dsound(main_window, 48000, 48000*sizeof(int16_t)*2);

            // get message
            while (global_running) {
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

                // This is for xbox controller and playstation controller won't work with this
                for (DWORD controller_index = 0; controller_index < XUSER_MAX_COUNT; controller_index++) {
                    XINPUT_STATE controller_state;
                    if (XInputGetState(controller_index, &controller_state) == ERROR_SUCCESS) {
                        // NOTE: This controller is plugged in
                        // TODO: See if ControllerState.dwPacketNumber increments too rapidly
                        XINPUT_GAMEPAD *game_pad = &controller_state.Gamepad;

                        bool up = (game_pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool down = (game_pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool left = (game_pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool right = (game_pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool start = (game_pad->wButtons & XINPUT_GAMEPAD_START);
                        bool back = (game_pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool left_shoulder = (game_pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool right_shoulder = (game_pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool a_button = (game_pad->wButtons & XINPUT_GAMEPAD_A);
                        bool b_button = (game_pad->wButtons & XINPUT_GAMEPAD_B);
                        bool x_button = (game_pad->wButtons & XINPUT_GAMEPAD_X);
                        bool y_button = (game_pad->wButtons & XINPUT_GAMEPAD_Y);
                        
                        int16_t stick_x = game_pad->sThumbLX;
                        int16_t stick_y = game_pad->sThumbLY;

                        x_offset += stick_x >> 12;
                        y_offset -= stick_y >> 12;

                        // if (a_button) {
                        //     y_offset += 3;
                        // }
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

                // this populates the bitmap memory object that we created
                render_wierd_gradient(&global_back_buffer, x_offset, y_offset);

                // ++x_offset;

                HDC device_context = GetDC(main_window);
                win32_window_dimension dimension = win32_get_window_dim(main_window);

                // this takes the memory object and populate the client rectangle
                win32_update_window(&global_back_buffer, device_context, dimension.width, dimension.height);
                ReleaseDC(main_window, device_context);
            }
        } else {
            // TODO: logging
        }
    } else {
        // TODO: logging
    }

    return 0;
}