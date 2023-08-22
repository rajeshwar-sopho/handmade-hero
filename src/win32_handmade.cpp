#include <windows.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define BYTES_PER_PIXEL 4

// typedef uint8_t uint8;

// TODO: global variable for now
global_variable bool running;

global_variable BITMAPINFO bitmap_info;
global_variable void *bitmap_memory;
global_variable int bitmap_width;
global_variable int bitmap_height;

internal void render_wierd_gradient(int x_offset, int y_offset) {
    int pitch = bitmap_width * BYTES_PER_PIXEL;
    uint8_t *row = (uint8_t*) bitmap_memory;
    for (int y=0; y < bitmap_height; ++y) {
        // uint32_t *pixel = (uint32_t*) row;
        uint32_t *pixel = (uint32_t*) row;
        
        for (int x=0; x < bitmap_width; ++x) {
            /*
                                  0  1  2  3
                Pixel in memory: 00 00 00 00
                                 BB GG RR xx
                LITTLE ENDIAN ARCHITECTURE: lower byte is placed first

                0x xxBBGGRR --> 
            */

           uint8_t blue = x - x_offset;
           uint8_t green = y - y_offset;
           uint8_t red = 0;
            
            *pixel++ = ((red << 16) | (green << 8) | blue);
        }

        row += pitch;
    } 
}


internal void win32_resize_DIB_section(int width, int height) {
    // TODO: Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails

    if (bitmap_memory) {
        VirtualFree(bitmap_memory, 0, MEM_RELEASE);
    }

    bitmap_height = height;
    bitmap_width = width;

    bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
    bitmap_info.bmiHeader.biWidth = bitmap_width;
    bitmap_info.bmiHeader.biHeight = -bitmap_height; // this will make windows draw from top to down se the biheight on msdn
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;
    
    int bitmap_memory_size = BYTES_PER_PIXEL * bitmap_width * bitmap_height;
    bitmap_memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
}

internal void win32_update_window(HDC device_context, RECT *client_rect) {
    int window_width = client_rect->right - client_rect->left;
    int window_height = client_rect->bottom - client_rect->top;


    StretchDIBits( device_context,
        /*
        x, y, width, height,
        x, y, width, height,
        */
        0, 0, bitmap_width, bitmap_height,
        0, 0, window_width, window_height,
        bitmap_memory,
        &bitmap_info,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

LRESULT CALLBACK mainwindow_callback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;
    switch(Msg) {
        case WM_SIZE:
        {
            RECT client_rect;
            GetClientRect(hWnd, &client_rect);
            LONG height = client_rect.bottom - client_rect.top;
            LONG width = client_rect.right - client_rect.left;
            win32_resize_DIB_section(width, height);
        } break;
        case WM_DESTROY:
        {
            running = false;
            OutputDebugStringA("WM_DESTROY");
        } break;
        case WM_CLOSE:
        {
            running = false;
            OutputDebugStringA("WM_CLOSE");
        } break;
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP");
        } break;
        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(hWnd, &paint);

            RECT client_rect;
            GetClientRect(hWnd, &client_rect);

            win32_update_window(device_context, &client_rect);

            EndPaint(hWnd, &paint);
        } break;
        default:
        {
            // OutputDebugStringA("WM_SIZE");
            result = DefWindowProc(hWnd, Msg, wParam, lParam) ;
        } break;
    }

    return result;
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
    // DisplayResourceNAMessageBox();
    WNDCLASSA wc = {};
    wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = mainwindow_callback;
    wc.hInstance = hInstance;
    // wc.hIcon = ;
    wc.lpszClassName = "HandmadeHeroWindowClass";
    
    if (RegisterClassA(&wc)) {
        // create a handle to the window wc
        HWND main_window_handle = CreateWindowExA(
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

        if (main_window_handle) {
            running = true;
            int x_offset, y_offset;
            x_offset = y_offset = 0;
            // get message
            while (running) {
                MSG message;
                // peek message loop when there is no message it will allow us to run instead of blocking
                while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
                    if (message.message == WM_QUIT) {
                        running = false;
                    }

                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }

                render_wierd_gradient(x_offset, y_offset);

                ++x_offset;
                y_offset -= 2;
                

                HDC device_context = GetDC(main_window_handle);
                RECT client_rect;
                GetClientRect(main_window_handle, &client_rect);

                int window_width = client_rect.right - client_rect.left;
                int window_height = client_rect.bottom - client_rect.top;

                win32_update_window(device_context, &client_rect);
                ReleaseDC(main_window_handle, device_context);
            }
        } else {
            // TODO: logging
        }
    } else {
        // TODO: logging
    }

    return 0;
}