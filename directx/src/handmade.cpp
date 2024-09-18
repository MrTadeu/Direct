#include "handmade.hpp"
// TODO: SEGMENTATION
/* void GameMain() {
    platform_window *window = PlatformOpenWindow("Handmade hero", int width, int height);
    platform_sound_device *Device = PlatformOpenSoundDevice();
}
void GameShutdown() {
    PlatformCloseSoundDevice(Device);
    PlatformCloseWindow(window);
} */

internal void GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz)
{
    local_persist float tSine;
    int16_t ToneVolume = 3000;
    /* int ToneHz = 256; */
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

    int16_t *SampleOut = SoundBuffer->Samples;
    for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex) {
        float SineValue = sinf(tSine);
        int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f * Pi32 * (float)1.0f / (float)WavePeriod;
    }
}

internal void RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
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

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer)
{
    local_persist int BlueOffset = 0;
    local_persist int GreenOffset = 0;
    local_persist int ToneHz = 256;

    if(Input.IsAnalog) {

    }
    else {

    }

    // Input.AButtonEndedDown;
    // Input.AButtonHalfTransitionCount;
    if (Input.AButtonEndedDown) {
        GreenOffset += 1;
    }

    GameOutputSound(SoundBuffer, ToneHz);
    RenderWeirdGradient(Buffer, BlueOffset, GreenOffset);
}
