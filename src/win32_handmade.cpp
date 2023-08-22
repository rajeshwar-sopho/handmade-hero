#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

// TODO: global variable for now
global_variable bool running;

global_variable BITMAPINFO bitmap_info;
global_variable void *bitmap_memory;
global_variable HBITMAP bitmap_handle;
global_variable HDC bitmap_device_context;

int DisplayResourceNAMessageBox()
{
    int msgboxID = MessageBoxA(
        NULL,
        "Resource not available\nDo you want to try again?",
        "Account Details",
        MB_ICONWARNING | MB_CANCELTRYCONTINUE | MB_DEFBUTTON2
    );

    switch (msgboxID)
    {
    case IDCANCEL:
        // TODO: add code
        break;
    case IDTRYAGAIN:
        // TODO: add code
        break;
    case IDCONTINUE:
        // TODO: add code
        break;
    }

    return msgboxID;
}


internal void win32_resize_DIB_section(
    int width, int height
) {
    // TODO: Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails
    
    // TODO: Free our DIBSection
    if (bitmap_handle) {
        DeleteObject(bitmap_handle);
    } 

    if (!bitmap_device_context) {
        // TODO: Should we recreate these under certain special circumstances
        bitmap_device_context = CreateCompatibleDC(0);
    }


    BITMAPINFO bitmap_info;
    bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
    bitmap_info.bmiHeader.biWidth = width;
    bitmap_info.bmiHeader.biHeight = height;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;
    

    void *bitmap_memory;
    bitmap_handle = CreateDIBSection(
        bitmap_device_context, &bitmap_info,
        DIB_RGB_COLORS, &bitmap_memory,
        0, 0);
}

internal void win32_update_window(
    HDC device_context, int x, int y, int width, int height
) {
    StretchDIBits( device_context,
        x, y, width, height,
        x, y, width, height,
        bitmap_memory,
        &bitmap_info,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

LRESULT CALLBACK mainwindow_callback(
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
) {
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

            RECT window_rect = paint.rcPaint;
            LONG x = window_rect.left;
            LONG y = window_rect.top;
            LONG height = window_rect.bottom - window_rect.top;
            LONG width = window_rect.right - window_rect.left;

            win32_update_window(device_context, x, y, width, height);

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
            // get messages
            while (running) {
                MSG messages;
                BOOL message_result = GetMessage(&messages, 0, 0, 0);
                if (message_result > 0) {
                    TranslateMessage(&messages);
                    DispatchMessage(&messages);
                } else {
                    break;
                }
            }
        } else {
            // TODO: logging
        }
    } else {
        // TODO: logging
    }

    return 0;
}