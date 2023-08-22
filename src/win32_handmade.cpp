#include <windows.h>

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
            OutputDebugStringA("WM_SIZE");
        } break;
        case WM_DESTROY:
        {
            OutputDebugStringA("WM_DESTROY");
        } break;
        case WM_CLOSE:
        {
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
            PatBlt(device_context, x, y, height, width, WHITENESS);
            
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
            // get messages
            while (TRUE) {
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