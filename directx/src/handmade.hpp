#pragma once

/* struct platform_window; //SEGMENTATION
platform_window *PlatformOpenWindow(char *title, int width, int height);

struct platform_sound_device; */


//? Services that the platform layer provides to the game

//? Servivices that the game provides to the platform layer
struct game_offscreen_buffer
{
    // pixels are always 32-bits wide, Memory Order BB GG RR XX
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int bytesPerPixel;
};

struct game_sound_output_buffer
{
    int SamplesPerSecond;
    int SampleCount;
    int16_t *Samples;
};

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer); // Timing, Controller/keyboard Input, Bitmap buffer, Sound buffer
