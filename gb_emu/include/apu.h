#pragma once

#include <common.h>

#define SAMPLE_RATE 44100;

typedef struct {
    u8 NR10;
    u8 NR30;
    u8 NRx1[4];
    u8 NRx2[4];
    u8 NRx3[4];
    u8 NRx4[4];
    u8 WaveRAM[16]; //FF30-FF3F
    u8 NR50;
    u8 NR51;
    u8 NR52;
} apu_context;

extern u32 apuSampleCount;
extern int16_t apuSamples[44100];

#define APU_POWERED_ON BIT(ctx.NR52, 7)

#define CHANNEL_1_ON BIT(ctx.NR52, 1)
#define CHANNEL_2_ON BIT(ctx.NR52, 2)
#define CHANNEL_3_ON BIT(ctx.NR52, 2)
#define CHANNEL_4_ON BIT(ctx.NR52, 3)

void apu_init();
void apu_tick();
void apu_sample();
void apu_clock_lengths();
void apu_clock_period_counters();

bool apu_get_dac_enabled(u8 channel);

void apu_write(u16 address, u8 value);
u8 apu_read(u16 address);

apu_context *apu_get_context();
