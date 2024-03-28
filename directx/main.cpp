#include <Windows.h>
#include <stdint.h>
#include <xinput.h>

#define internal static 
#define local_persist static 
#define global_variable static 

struct Win32_offscreen_buffer {
	// pixels are always 32-bits wide, Memory Order BB GG RR XX
	BITMAPINFO Info;
	void* Memory;
	int Width;
	int Height;
	int Pitch;
};


//TODO: This is a global for now.
global_variable bool GlobalRunning;
global_variable Win32_offscreen_buffer GlobalBackbuffer;

struct win32_window_dimensions {
	int Width;
	int Height;
};

// XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);

X_INPUT_GET_STATE(XInputGetStateStub) {
	return ERROR_DEVICE_NOT_CONNECTED;
}

// XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) {
	return ERROR_DEVICE_NOT_CONNECTED;
}

global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;
global_variable x_input_set_state* XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

internal void Win32LoadXInput(void) {
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (XInputLibrary) {
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
	}
}

internal win32_window_dimensions Win32GetWindowDimension(HWND Window) {
	win32_window_dimensions Result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return Result;
}

internal void RenderWeirdGradient(Win32_offscreen_buffer Buffer, int BlueOffset, int GreenOffset) {
	/*
				    Row						width*BytesPerPixel  == valor de uma linha
	BitmapMemory ->   0  BB GG RR xx BB GG RR xx BB GG RR xx ...
					1  BB GG RR xx BB GG RR xx BB GG RR xx ...
					2  BB GG RR xx BB GG RR xx BB GG RR xx ...
		
		Pitch + BitmapMemory  (O Pitch nos move para a proxima linha)
	*/
	//TODO: Let's see what the optimizes does
	uint8_t* Row = (uint8_t*)Buffer.Memory; 
	for (int Y = 0; Y < Buffer.Height; ++Y) {
		
		uint32_t* Pixel = (uint32_t*)Row;
		for (int X = 0; X < Buffer.Width; ++X) {
			uint8_t Blue = (X + BlueOffset);
			uint8_t Green = (Y + GreenOffset);
			
			*Pixel++ = (Green << 8 | Blue);
			
		}

		Row += Buffer.Pitch; // tbm dava para fazer  Row = (uint8_t)Pixel;
	}
}


//Device Independent Bitmap -> Contem a "color table"
internal void Win32ResizeDIBSection(Win32_offscreen_buffer *Buffer, int width, int height) {
	// TODO: Bulletproof this!
	if (Buffer->Memory) {
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}
	Buffer->Width = width;
	Buffer->Height = height;
	int BytesPerPixel = 4;
		  
	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	// No more DC for us.
	int BitmapMemorySize = (Buffer->Height * Buffer->Width) * BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
	Buffer->Pitch = width * BytesPerPixel; // � a linha de: BB GG RR xx BB GG RR xx BB GG RR xx ...

	//TODO: Probably clear this to black
}

internal void Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight, Win32_offscreen_buffer Buffer) {
	//TODO: Aspect ratio correction!
	StretchDIBits(DeviceContext,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer.Width, Buffer.Height,
        Buffer.Memory, &Buffer.Info,
        DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK Win32MainWindowCallback(
    HWND Window, 
    UINT Message,
    WPARAM WParam,
    LPARAM LParam)
{
	LRESULT Result = 0;

	switch (Message){
		case WM_SIZE: {
			OutputDebugStringA("WM_SIZE\n");
        }
		break;
		case WM_CLOSE: {
            //TODO: Handle this with a message to the user
			GlobalRunning = false; /*Em vez de PostQuitMessage(0) e/ou DestroyWindow(window)*/
        }
		break;
		case WM_ACTIVATEAPP:{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
        }
		break;
		case WM_DESTROY:{
			//TODO: Handle this as an error - recreate window?
			GlobalRunning = false;
        }
		break;
		case WM_SYSKEYDOWN: {

		}
		break;
		case WM_SYSKEYUP: {

		}
		break;
		case WM_KEYDOWN: {
			
		}
		break;
		case WM_KEYUP: {
			uint32_t VKCode = WParam;
			if (VKCode == 'W') {
				OutputDebugStringA("W");
			}
			LParam & (1 << 30);
		}
		case WM_PAINT:{
            PAINTSTRUCT Paint;
			HDC  DeviceContext = BeginPaint( Window, &Paint);
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.right;
			int width = Paint.rcPaint.right - Paint.rcPaint.left;
			int height = Paint.rcPaint.bottom - Paint.rcPaint.top;
			
			win32_window_dimensions Dimension = Win32GetWindowDimension(Window);
			Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackbuffer);
			EndPaint(Window, &Paint);
        }
		break;
		default: {
			Result = DefWindowProc(Window, Message, WParam, LParam);
        }
		break;
	}
	return Result;
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR ComandLine, int ShowCode){
	Win32LoadXInput();
	WNDCLASS windowClass = { 0 };
	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc = Win32MainWindowCallback;
	windowClass.hInstance = Instance; 
	//windowClass.hCursor = ; // nesse jogo n�o vai ter cursor do windows!
	windowClass.lpszClassName = TEXT("HandMadeHeroWindowClass");

	if (RegisterClass(&windowClass)) {
		HWND Window = CreateWindowEx(0, windowClass.lpszClassName, TEXT("Handmade hero"), WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);
		if (Window) {
			int XOffset = 0;
			int YOffset = 0;
			GlobalRunning = true;
			HDC DeviceContext = GetDC(Window); //nao precisa de ReleaseDC pq CS_OWNDC
			while (GlobalRunning) { 
				MSG Message;
				while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
					if (Message.message == WM_QUIT) {
						GlobalRunning = false;
					}
					TranslateMessage(&Message);
					DispatchMessageA(&Message); 
				}

				for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++) {
					XINPUT_STATE ControllerState;
					if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {
						XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;
						bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
						bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
						bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
						bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
						bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

						int16_t StickX = Pad->sThumbLX;
						int16_t StickY = Pad->sThumbLY;

						if (AButton) {
							XINPUT_VIBRATION vibration;
							vibration.wLeftMotorSpeed = 60000;
							vibration.wRightMotorSpeed = 60000;

							XInputSetState(0, &vibration);
							YOffset += 2;
						}
						else {
							XINPUT_VIBRATION vibration;
							vibration.wLeftMotorSpeed = 0;
							vibration.wRightMotorSpeed = 0;

							XInputSetState(0, &vibration);
						}
					}
					else {
						//controle nao conectado
					}
				}
				

				RenderWeirdGradient(GlobalBackbuffer, XOffset, YOffset);

				win32_window_dimensions Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackbuffer);

				++XOffset;
			}
		}
		else {
			//TODO: Logging
		}
	}
	else {
		//TODO: Logging
	}

	return 0;
}	
