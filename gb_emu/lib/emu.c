#include <stdio.h>
#include <emu.h>
#include <cart.h>
#include <cpu.h>
#include <ui.h>
#include <timer.h>
#include <dma.h>
#include <ppu.h>
#include <apu.h>

#include <SDL2/SDL.h>

//TODO Add Windows Alternative...
#include <pthread.h>
#include <unistd.h>

/* 
  Emu components:

  |Cart|
  |CPU|
  |Address Bus|
  |PPU|
  |Timer|

*/

#define BUFFER_SIZE (735 * 2)

int16_t audioBuffer[BUFFER_SIZE];
volatile int bufferWritePos = 0; // May want to change these and properly handle overflow logic
volatile int bufferReadPos = 0;
SDL_mutex *bufferMutex;

static emu_context ctx;

emu_context *emu_get_context() {
    return &ctx;
}

void *cpu_run(void *p) {
    timer_init();
    cpu_init();
    ppu_init();

    ctx.running = true;
    ctx.paused = false;
    ctx.ticks = 0;

    while(ctx.running) {
        if (ctx.paused) {
            delay(10);
            continue;
        }

        if (!cpu_step()) {
            printf("CPU Stopped\n");
            return 0;
        }
    }

    return 0;
}

void pushAudioSamples(const int16_t* samples, int sampleCount)
{
    SDL_LockMutex(bufferMutex);

    for (int i = 0; i < sampleCount; i++)
    {
        audioBuffer[bufferWritePos % BUFFER_SIZE] = samples[i];
        bufferWritePos++;
    }

    SDL_UnlockMutex(bufferMutex);
}

void audioCallback(void* userdata, u8* stream, int len)
{
    int16_t* buffer = (int16_t*)stream;

    int samplesToCopy = len / sizeof(int16_t);

    SDL_LockMutex(bufferMutex);

    for (int i = 0; i < samplesToCopy; i++)
    {
        if (bufferReadPos < bufferWritePos)
        {
            buffer[i] = audioBuffer[bufferReadPos % BUFFER_SIZE];
            bufferReadPos++;
        }
        else
        {
            buffer[i] = 0;
        }
    }

    SDL_UnlockMutex(bufferMutex);
}

int emu_run(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: emu <rom_file>\n");
        return -1;
    }

    if (!cart_load(argv[1])) {
        printf("Failed to load ROM file: %s\n", argv[1]);
        return -2;
    }

    printf("Cart loaded..\n");

    ui_init();
    
    pthread_t t1;

    if (pthread_create(&t1, NULL, cpu_run, NULL)) {
        fprintf(stderr, "FAILED TO START MAIN CPU THREAD!\n");
        return -1;
    }

    SDL_AudioSpec audioSpec;

    SDL_zero(audioSpec);
    audioSpec.freq = SAMPLE_RATE;
    audioSpec.format = AUDIO_S16SYS;
    audioSpec.channels = 1; // This can probably be changed to stereo in the future
    audioSpec.samples = 4096;
    audioSpec.callback = audioCallback;

    if (SDL_OpenAudio(&audioSpec, NULL) < 0)
    {
        fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    bufferMutex = SDL_CreateMutex();

    SDL_PauseAudio(0);

    u32 prev_frame = 0;

    while(!ctx.die) {
        usleep(1000);
        ui_handle_events();

        if (prev_frame != ppu_get_context()->current_frame) {
            pushAudioSamples(apuSamples, apuSampleCount);
            apuSampleCount = 0;
            ui_update();
        }

        prev_frame = ppu_get_context()->current_frame;
    }

    return 0;
}

void emu_cycles(int cpu_cycles) {
    
    for (int i=0; i<cpu_cycles; i++) {
        for (int n=0; n<4; n++) { //T-Cycle
            ctx.ticks++;
            timer_tick();
            ppu_tick();
            apu_tick();
        }

        dma_tick();
    }
}
