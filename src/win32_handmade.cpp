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

    // doing this makes the pointer math easier as it prevents c from multiplying 
    // the values with the size of variable
    uint8_t *row = (uint8_t*) bitmap_memory;
    for (int y=0; y < bitmap_height; ++y) {
        // uint32_t *pixel = (uint32_t*) row;
        uint32_t *pixel = (uint32_t*) row;
        
        for (int x=0; x < bitmap_width; ++x) {
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

    // this formula and details about the bitmap header are mentioned on below page
    // https://learn.microsoft.com/en-us/windows/win32/gdi/device-independent-bitmaps
    int bitmap_memory_size = BYTES_PER_PIXEL * bitmap_width * bitmap_height;

    bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
    bitmap_info.bmiHeader.biWidth = bitmap_width;
    bitmap_info.bmiHeader.biHeight = -bitmap_height; // this will make windows draw from top to down se the biheight on msdn
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;
    
    // virtual alloc is faster as malloc will go through a bunch of code and eventually call
    // virtualalloc, as eventually we need memory in the form of pages where a page is just a byte
    // windows allocates virtual memory to the program not actual memory for safety
    bitmap_memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
}

internal void win32_update_window(HDC device_context, RECT *client_rect) {
    int window_width = client_rect->right - client_rect->left;
    int window_height = client_rect->bottom - client_rect->top;


    // its going to copy bitmap memory object to the window and its in rgb colors
    // since bitmap_height and width are global and every time we resize the window
    // we are setting them equal to the h and w of the window, they are same
    StretchDIBits( device_context,
        /*
        x, y, width, height,
        x, y, width, height,
        */
        0, 0, window_width, window_height, // which is pointed by the device_context
        0, 0, bitmap_width, bitmap_height, // these are the dimensions for memory where bitmap data is stored
        bitmap_memory,
        &bitmap_info,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

LRESULT CALLBACK mainwindow_callback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    // this function is a call back that processes messages received by the window
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
            // so the paint object is giving us both, DC and the rectangle to draw on
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(hWnd, &paint);

            // render_wierd_gradient(100, 0);
            win32_update_window(device_context, &paint.rcPaint);

            EndPaint(hWnd, &paint);
        } break;
        default:
        {
            // OutputDebugStringA("WM_SIZE");
            // for messages that you dont want to process or want window's default behavior call this
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
    
    // usually there is no hard need to release the resources and just closing is fine
    // as windows will clear that automatically for us when someone will close the app
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

                // this populates the bitmap memory object that we created
                render_wierd_gradient(x_offset, y_offset);

                ++x_offset;
                y_offset -= 2;
                

                HDC device_context = GetDC(main_window_handle);
                RECT client_rect;
                GetClientRect(main_window_handle, &client_rect);

                int window_width = client_rect.right - client_rect.left;
                int window_height = client_rect.bottom - client_rect.top;

                // this takes the memory object and populate the client rectangle
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