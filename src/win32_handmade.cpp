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

// they stick in the kernel code so we can call functions from there
// requirement and library is mentioned in the msdn page at the bottom
int WINAPI WinMain(
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance,
    PSTR lpCmdLine, 
    int nCmdShow)
{
    DisplayResourceNAMessageBox();
    return 0;
}