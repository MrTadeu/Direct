/**
 * TODO:
 *
 * 1. Save game locations
 * 2. Getting a handle to our own executable file
 * 3. Asset loading path
 * 4. Threading (launch a thread)
 * 5. Raw Input (support for multiple keyboards)
 * 6. Sleep/timeBeginPeriod
 * 7. ClipCursor() (for multimonitor support)
 * 8. Fullscreen support
 * 9. WM_SETCURSOR (control cursor visibility)
 * 10. QueryCancelAutoplay
 * 11. WM_ACTIVATEAPP (for when we are not the active application)
 * 12. Blit speed improvements (BitBlt)
 * 13. Hardware acceleration (OpenGL or Direct3D or Vulkan)
 * 14. GetKeyboardLayout (for international keyboards)

 just a partial list of stuff!!
 */

#include <Windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>
#include <malloc.h>

// to use the OutputDebugStringA and format the output
#include <sstream>
#include <iostream>
#include <iomanip>

#include "win32_handmade.hpp"

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

// Print output (Plataform independent but dependent on the compiler)
#ifdef VISUAL_STUDIO_OUTPUTCONSOLE
#define Print(x)                              \
	{                                         \
		std::stringstream ss;                 \
		ss << x;                              \
		OutputDebugStringA(ss.str().c_str()); \
	}
#else
#define Print(x) std::cout << x
#endif

#define fFormat(x) std::fixed << std::setprecision(x)

#include "handmade.cpp"


// TODO: This is a global for now.
global_variable bool GlobalRunning;
global_variable Win32_offscreen_buffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

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

/* void *PlataformLoadFile(const char *filename){ //TODO: SEGMENTATION
	// win32 specific code
	return 0;
} */

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

// Device Independent Bitmap -> Contem a "color table"
internal void Win32ResizeDIBSection(Win32_offscreen_buffer *Buffer, int width, int height)
{
	// TODO: Bulletproof this!
	if (Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}
	int BytesPerPixel = 4;
	Buffer->Width = width;
	Buffer->Height = height;
	Buffer->bytesPerPixel = BytesPerPixel;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = (WORD)(Buffer->bytesPerPixel * 8);
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	Buffer->Pitch = width * Buffer->bytesPerPixel; // a linha de: BB GG RR xx BB GG RR xx BB GG RR xx ...

	int BitmapMemorySize = Buffer->Pitch * Buffer->Height;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	// TODO: Probably clear this to black
}

internal void Win32DisplayBufferInWindow(Win32_offscreen_buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight)
{
	// TODO: Aspect ratio correction!
	StretchDIBits(DeviceContext,
				  0, 0,
				  Buffer->Width, Buffer->Height, /*OLD: WindowWidth, WindowHeight, */
				  0, 0,
				  Buffer->Width, Buffer->Height,
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
		bool AltKeyWasDown = (LParam & (1 << 29));
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

internal void win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite, game_sound_output_buffer *SourceBuffer)
{
	void *Region1;
	DWORD Region1Size;
	void *Region2;
	DWORD Region2Size;
	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
	{
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
		int16_t *DestSample = (int16_t *)Region1;
		int16_t *SourceSample = SourceBuffer->Samples;
		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}

		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
		DestSample = (int16_t *)Region2;
		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal void win32ClearBuffer(win32_sound_output *SoundOutput)
{
	void *Region1;
	DWORD Region1Size;
	void *Region2;
	DWORD Region2Size;
	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
	{
		uint8_t *DestSample = (uint8_t *)Region1;
		for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
		{
			*DestSample++ = 0;
		}

		DestSample = (uint8_t *)Region2;
		for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
		{
			*DestSample++ = 0;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR ComandLine, int ShowCode)
{
	/* GameMain(); */ // SEGMENTATION

	LARGE_INTEGER PerformanceCountFrequencyResult;
	QueryPerformanceFrequency(&PerformanceCountFrequencyResult);
	int64_t PerformanceCountFrequency = PerformanceCountFrequencyResult.QuadPart; // counts per second

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

			win32_sound_output SoundOutput = {};

			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.RunningSampleIndex = 0;
			// [ 16   16 BITS]
			// [ L ][ R ]   [ L ][ R ]    [ L ][ R ]....
			SoundOutput.BytesPerSample = sizeof(int16_t) * 2; //  *2 pq existem 2 canais L e R
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			win32ClearBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			GlobalRunning = true;

			int16_t *Samples = (int16_t *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			LARGE_INTEGER LastCounter;
			QueryPerformanceCounter(&LastCounter);
			uint64_t LastCycleCount = __rdtsc(); // RDTSC - Read Time Stamp Counter - From intel architecture reference manual volumn 2 (charpter 3/4)
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

						/* XOffset += StickX / XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
						YOffset -= StickY / XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE; */

						if (AButton)
						{
							XINPUT_VIBRATION vibration;
							vibration.wLeftMotorSpeed = 60000;
							vibration.wRightMotorSpeed = 60000;

							XInputSetState(0, &vibration);
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

				// DirectSound 
				DWORD ByteToLock = 0;
				DWORD TargetCursor = 0;
				DWORD BytesToWrite = 0;
				DWORD PlayCursor = 0;
				DWORD WriteCursor = 0;
				bool SoundIsValid = false;
				if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
				{

					ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
					TargetCursor = (PlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize;
					if (ByteToLock > TargetCursor)
					{
						BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
						BytesToWrite += TargetCursor;
					}
					else
					{
						BytesToWrite = TargetCursor - ByteToLock;
					}
					SoundIsValid = true;
				}

				game_sound_output_buffer SoundBuffer = {};
				SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
				SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
				SoundBuffer.Samples = Samples;

				game_offscreen_buffer Buffer = {};
				Buffer.Memory = GlobalBackbuffer.Memory;
				Buffer.Width = GlobalBackbuffer.Width;
				Buffer.Height = GlobalBackbuffer.Height;
				Buffer.Pitch = GlobalBackbuffer.Pitch;
				GameUpdateAndRender(&Buffer, &SoundBuffer);

				if (SoundIsValid)
				{
					win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
				}

				win32_window_dimensions Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);

				uint64_t EndCycleCount = __rdtsc(); // its a intrinsic "function" (not a function but a direct register access)

				LARGE_INTEGER EndCounter;
				QueryPerformanceCounter(&EndCounter);

				int64_t CyclesElapsed = EndCycleCount - LastCycleCount;
				int64_t CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart; // counts per frame
				float MSPerFrame = (float)((CounterElapsed * 1000.0f) / PerformanceCountFrequency);
				int32_t FPS = (int32_t)(PerformanceCountFrequency / CounterElapsed);
				float MCPF = (float)(CyclesElapsed / (1000.0f * 1000.0f));

				Print(fFormat(2) << MSPerFrame << "ms/f, " << FPS << "f/s, " << MCPF << "mc/f\n");

				LastCounter = EndCounter;
				LastCycleCount = EndCycleCount;

				/**
				 * @brief SIND (MULTIPLES OPERATIONS IN A REGISTER)
				 * S - single
				 * I - instruction
				 * M - multiple
				 * D - data
				 */
				// info from agner optimizations
				// float - MULPS -> 128 BITS / 32 BITS => 4 FLOATS into the register | 5 cycles to complete, 1 cycle throughput
				// double - MULPD -> 128 BITS / 64 BITS => 2 FLOATS into the register | 5 cycles to complete, 1 cycle throughput
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
