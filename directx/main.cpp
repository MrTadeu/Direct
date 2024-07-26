#include <Windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int32_t bool32;

struct Win32_offscreen_buffer
{
	// pixels are always 32-bits wide, Memory Order BB GG RR XX
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};

// TODO: This is a global for now.
global_variable bool GlobalRunning;
global_variable Win32_offscreen_buffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

struct win32_window_dimensions
{
	int Width;
	int Height;
};

// XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);

X_INPUT_GET_STATE(XInputGetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void Win32LoadXInput(void)
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (!XInputLibrary)
	{
		// TODO: Diagnostic
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
	}
	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
	}
	else
	{
		// TODO: Diagnostic
	}
}

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void Win32InitDSound(HWND Window, int32_t samplesPerSecond, int32_t BufferSize)
{
	// Load the library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
	if (DSoundLibrary)
	{
		// Get a DirectSound object!
		direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		WAVEFORMATEX WaveFormat = {};
		WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
		WaveFormat.nChannels = 2;
		WaveFormat.nSamplesPerSec = samplesPerSecond;
		WaveFormat.wBitsPerSample = 16;
		WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
		WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
		WaveFormat.cbSize = 0;

		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
				BufferDescription.lpwfxFormat = 0;

				// "create" a primary buffer
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					WAVEFORMATEX WaveFormat = {};
					WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
					WaveFormat.nChannels = 2;
					WaveFormat.nSamplesPerSec = samplesPerSecond;
					WaveFormat.wBitsPerSample = 16;
					WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
					WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
					WaveFormat.cbSize = 0;
					if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
					{
						// We have finally set the format!
						OutputDebugStringA("Primary buffer formant was set.\n");
					}
					else
					{
						// TODO: Diagnostic
					}
				}
			}
			else
			{
				// TODO: Diagnostic
			}
			/* code */
		}
		else
		{
			// TODO: Diagnostic
		}

		// "create" a secondary buffer
		DSBUFFERDESC BufferDescription = {};
		BufferDescription.dwSize = sizeof(BufferDescription);
		BufferDescription.dwFlags = 0;
		BufferDescription.lpwfxFormat = &WaveFormat;
		BufferDescription.dwBufferBytes = BufferSize;
		if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
		{
		}
		// start it playing
	}
	else
	{
		// TODO: Diagnostic
	}
}

internal win32_window_dimensions Win32GetWindowDimension(HWND Window)
{
	win32_window_dimensions Result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return Result;
}

internal void RenderWeirdGradient(Win32_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
	/*
					Row						width*BytesPerPixel  == valor de uma linha
	BitmapMemory ->   0  BB GG RR xx BB GG RR xx BB GG RR xx ...
					1  BB GG RR xx BB GG RR xx BB GG RR xx ...
					2  BB GG RR xx BB GG RR xx BB GG RR xx ...

		Pitch + BitmapMemory  (O Pitch nos move para a proxima linha)
	*/
	// TODO: Let's see what the optimizes does
	uint8_t *Row = (uint8_t *)Buffer->Memory;
	for (int Y = 0; Y < Buffer->Height; ++Y)
	{

		uint32_t *Pixel = (uint32_t *)Row;
		for (int X = 0; X < Buffer->Width; ++X)
		{
			uint8_t Blue = (X + BlueOffset);
			uint8_t Green = (Y + GreenOffset);

			*Pixel++ = (Green << 8 | Blue);
		}

		Row += Buffer->Pitch; // tbm dava para fazer  Row = (uint8_t)Pixel;
	}
}

// Device Independent Bitmap -> Contem a "color table"
internal void Win32ResizeDIBSection(Win32_offscreen_buffer *Buffer, int width, int height)
{
	// TODO: Bulletproof this!
	if (Buffer->Memory)
	{
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
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Buffer->Pitch = width * BytesPerPixel; // � a linha de: BB GG RR xx BB GG RR xx BB GG RR xx ...

	// TODO: Probably clear this to black
}

internal void Win32DisplayBufferInWindow(Win32_offscreen_buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight)
{
	// TODO: Aspect ratio correction!
	StretchDIBits(DeviceContext,
				  0, 0, WindowWidth, WindowHeight,
				  0, 0, Buffer->Width, Buffer->Height,
				  Buffer->Memory, &Buffer->Info,
				  DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK Win32MainWindowCallback(
	HWND Window,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam)
{
	LRESULT Result = 0;

	switch (Message)
	{
	case WM_SIZE:
	{
		OutputDebugStringA("WM_SIZE\n");
	}
	break;
	case WM_CLOSE:
	{
		// TODO: Handle this with a message to the user
		GlobalRunning = false; /*Em vez de PostQuitMessage(0) e/ou DestroyWindow(window)*/
	}
	break;
	case WM_ACTIVATEAPP:
	{
		OutputDebugStringA("WM_ACTIVATEAPP\n");
	}
	break;
	case WM_DESTROY:
	{
		// TODO: Handle this as an error - recreate window?
		GlobalRunning = false;
	}
	break;
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		uint32_t VKCode = WParam;
		bool WasDown = ((LParam & (1 << 30)) != 0);
		bool IsDown = ((LParam & (1 << 31)) == 0);
		if (IsDown != WasDown)
		{
			if (VKCode == 'W')
			{
				OutputDebugStringA("W\n");
			}
			else if (VKCode == 'A')
			{
				OutputDebugStringA("A\n");
			}
			else if (VKCode == 'S')
			{
				OutputDebugStringA("S\n");
			}
			else if (VKCode == 'D')
			{
				OutputDebugStringA("D\n");
			}
			else if (VKCode == 'Q')
			{
				OutputDebugStringA("Q\n");
			}
			else if (VKCode == 'E')
			{
				OutputDebugStringA("E\n");
			}
			else if (VKCode == VK_UP)
			{
				OutputDebugStringA("UP\n");
			}
			else if (VKCode == VK_DOWN)
			{
				OutputDebugStringA("DOWN\n");
			}
			else if (VKCode == VK_LEFT)
			{
				OutputDebugStringA("LEFT\n");
			}
			else if (VKCode == VK_RIGHT)
			{
				OutputDebugStringA("RIGHT\n");
			}
			else if (VKCode == VK_ESCAPE)
			{
				OutputDebugStringA("ESCAPE\n");
				if (IsDown)
				{
					OutputDebugStringA("IsDown");
				}
				if (WasDown)
				{
					OutputDebugStringA("WasDown");
				}
				OutputDebugStringA("\n");
			}
			else if (VKCode == VK_SPACE)
			{
				OutputDebugStringA("SPACE\n");
			}
		}
		bool32 AltKeyWasDown = (LParam & (1 << 29));
		if (VKCode == VK_F4 && AltKeyWasDown)
		{
			GlobalRunning = false;
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT Paint;
		HDC DeviceContext = BeginPaint(Window, &Paint);
		int X = Paint.rcPaint.left;
		int Y = Paint.rcPaint.right;
		int width = Paint.rcPaint.right - Paint.rcPaint.left;
		int height = Paint.rcPaint.bottom - Paint.rcPaint.top;

		win32_window_dimensions Dimension = Win32GetWindowDimension(Window);
		Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
		EndPaint(Window, &Paint);
	}
	break;
	default:
	{
		Result = DefWindowProc(Window, Message, WParam, LParam);
	}
	break;
	}
	return Result;
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR ComandLine, int ShowCode)
{
	Win32LoadXInput();
	WNDCLASSA windowClass = {0};
	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc = Win32MainWindowCallback;
	windowClass.hInstance = Instance;
	// windowClass.hCursor = ; // nesse jogo n�o vai ter cursor do windows!
	windowClass.lpszClassName = "HandMadeHeroWindowClass";

	if (RegisterClassA(&windowClass))
	{
		HWND Window = CreateWindowExA(0, windowClass.lpszClassName, "Handmade hero", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);
		if (Window)
		{
			HDC DeviceContext = GetDC(Window); // nao precisa de ReleaseDC pq CS_OWNDC

			// Graphics test
			int XOffset = 0;
			int YOffset = 0;

			// Sound test
			int samplesPerSecond = 48000;
			int ToneHz = 256;
			int16_t ToneVolume = 2000;
			uint32_t RunningSampleIndex = 0;
			int SquareWavePeriod = samplesPerSecond / ToneHz;
			int HalfSquareWavePeriod = SquareWavePeriod / 2;
			// [ 16   16 BITS]
			// [ L ][ R ]   [ L ][ R ]    [ L ][ R ]....
			int BytesPerSample = sizeof(int16_t) * 2;
			int SecondaryBufferSize = samplesPerSecond * BytesPerSample;

			Win32InitDSound(Window, samplesPerSecond, SecondaryBufferSize);
			bool32 SoundIsPlaying = false;

			GlobalRunning = true;
			while (GlobalRunning)
			{
				MSG Message;
				while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
				{
					
					if (Message.message == WM_QUIT)
					{
						GlobalRunning = false;
					}
					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}

				for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++)
				{
					XINPUT_STATE ControllerState;
					if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
					{
						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
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

						XOffset += StickX >> 12;
						YOffset -= StickY >> 12;

						if (AButton)
						{
							XINPUT_VIBRATION vibration;
							vibration.wLeftMotorSpeed = 60000;
							vibration.wRightMotorSpeed = 60000;

							XInputSetState(0, &vibration);
							YOffset += 2;
						}
						else
						{
							XINPUT_VIBRATION vibration;
							vibration.wLeftMotorSpeed = 0;
							vibration.wRightMotorSpeed = 0;

							XInputSetState(0, &vibration);
						}
					}
					else
					{
						// controle nao conectado
					}
				}

				RenderWeirdGradient(&GlobalBackbuffer, XOffset, YOffset);

				// DirectSound output test
				DWORD PlayCursor;
				DWORD WriteCursor;
				if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
				{
					DWORD BytesToLock = RunningSampleIndex * BytesPerSample % SecondaryBufferSize;
					DWORD BytesToWrite;
					if (BytesToLock == PlayCursor)
					{
						BytesToWrite = SecondaryBufferSize;
					}
					else {
						if (BytesToLock > PlayCursor)
						{
							BytesToWrite = (SecondaryBufferSize - BytesToLock);
							BytesToWrite += PlayCursor;
						}
						else
						{
							BytesToWrite = PlayCursor - BytesToLock;
						}
					}

					void *Region1;
					DWORD Region1Size;
					void *Region2;
					DWORD Region2Size;
					if (SUCCEEDED(GlobalSecondaryBuffer->Lock(BytesToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
					{
						int16_t *SampleOut = (int16_t *)Region1;
						DWORD Region1SampleCount = Region1Size / BytesPerSample;
						for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
						{
							int16_t SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? ToneVolume : -ToneVolume;
							*SampleOut++ = SampleValue;
							*SampleOut++ = SampleValue;
						}
						SampleOut = (int16_t *)Region2;
						DWORD Region2SampleCount = Region2Size / BytesPerSample;
						for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
						{
							int16_t SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? ToneVolume : -ToneVolume;
							*SampleOut++ = SampleValue;
							*SampleOut++ = SampleValue;
						}
						GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
					}
				}
				if (!SoundIsPlaying)
				{
					SoundIsPlaying = true;
					GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
				}

				win32_window_dimensions Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
			}
		}
		else
		{
			// TODO: Logging
		}
	}
	else
	{
		// TODO: Logging
	}

	return 0;
}
