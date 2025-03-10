#include <apu.h>
#include <interrupts.h>
#include <math.h>
#include <timer.h>
#include <SDL2/SDL.h>

#define APU_DEBUG 0

static apu_context ctx = {0};

static u8 dutyCycles[4][8] = {  {0, 0, 0, 0, 0, 0, 0, 1},
                                {0, 0, 0, 0, 0, 0, 1, 1},
                                {0, 0, 0, 0, 1, 1, 1, 1},
                                {1, 1, 1, 1, 1, 1, 0, 0} };

u32 apuSampleCount = 0;
int16_t apuSamples[44100];

bool trigger[4] = {0};
u32 sample = 0;
u8 paceRead = 0;
u8 shadowRegister = 0;
u16 divAPU = 0;
bool prevDiv4 = 0;
u16 periodCounter[4] = {0};
u8 sampleIndex[4] = {0};
u32 tick = 0;

apu_context *apu_get_context() {
    return &ctx;
}

void apu_init() {
}

void apu_tick() {
    if (!APU_POWERED_ON)
    {
        ctx.NR52 &= ~(0b1111); // If powered off, power off all channels
        ctx.NR10 = 0;
        ctx.NR30 = 0;

        for (int i = 0; i < 4; i++)
        {
            ctx.NRx1[i] = 0;
            ctx.NRx2[i] = 0;
            ctx.NRx3[i] = 0;
            ctx.NRx4[i] = 0;
        }
        ctx.NR50 = 0;
        ctx.NR51 = 0;
    }
    else
    {
        if ((timer_get_context()->div, 4) != prevDiv4)
        {
            divAPU++;

            u8 step = divAPU % 8;

            if ((step % 2) == 0) //Even steps
            {
                //apu_clock_lengths();
            }

            if (step == 7)
            {
               //apu_clock_envelope();
            }

            if ((step % 4) == 2) // 2 and 6 steps
            {
                //apu_clock_sweep();
            }
        }
        prevDiv4 = BIT(timer_get_context()->div, 4);

        apu_clock_period_counters();

        if (trigger[0])
        { 
            printf("Channel 1 triggered\n");
            ctx.NR52 |= 1 << 0;
            trigger[0] = false;
        }

        if (trigger[1])
        { 
            printf("Channel 2 triggered\n");
            ctx.NR52 |= 1 << 1;
            trigger[1] = false;
        }

        if (BIT(ctx.NR52, 1))
        {
            
        }
    }

    //this is every 1/1048576 seconds
    //want to sample every 1/44100 seconds
    tick++;

    u32 SAMPLE_PERIOD = 1048576 / SAMPLE_RATE;

    if ((tick % SAMPLE_PERIOD) == 0)
    {
        apu_sample();
    }
}

void apu_sample()
{
    int16_t sample1 = 10000 * dutyCycles[(ctx.NRx1[0] >> 6) & 0x3][sampleIndex[0]] * BIT(ctx.NR52, 0);
    int16_t sample2 = 10000 * dutyCycles[(ctx.NRx1[1] >> 6) & 0x3][sampleIndex[1]] * BIT(ctx.NR52, 1);

    int16_t mixed = (sample1 + sample2) / 2;

    apuSamples[apuSampleCount] = mixed;

    apuSampleCount++;
}

void apu_clock_period_counters()
{
    u16 period1 = 0x800 - (ctx.NRx3[0] + ((ctx.NRx4[0] & 0b111) << 8));
    u16 period2 = 0x800 - (ctx.NRx3[1] + ((ctx.NRx4[1] & 0b111) << 8));
    u16 period3 = 0x800 - (ctx.NRx3[2] + ((ctx.NRx4[2] & 0b111) << 8));
    u16 period4 = 0x800 - (ctx.NRx3[3] + ((ctx.NRx4[3] & 0b111) << 8));


    if (BIT(ctx.NR52, 0))
    {
        if (periodCounter[0] == 2047)
        {
            periodCounter[0] = period1;

            sampleIndex[0]++;

            if (sampleIndex[0] == 8)
            {
                sampleIndex[0] = 0;
            }
        }

        periodCounter[0]++;
    }

    if (BIT(ctx.NR52, 1))
    {
        if (periodCounter[1] == 2047)
        {
            periodCounter[1] = period2;

            sampleIndex[1]++;

            if (sampleIndex[1] == 8)
            {
                sampleIndex[1] = 0;
            }
        }

        periodCounter[1]++;
    }

    if (BIT(ctx.NR52, 3))
    {
        if (periodCounter[3] == 2047)
        {
            periodCounter[3] = period4;

            sampleIndex[3]++;

            if (sampleIndex[3] == 8)
            {
                sampleIndex[3] = 0;
            }
        }

        periodCounter[3]++;
    }
}

void apu_clock_lengths()
{
    if (BIT(ctx.NRx4[0], 6)) // Increment length timer
    {
        ctx.NRx1[0] = ctx.NRx1[0] & 0xC0 + (ctx.NRx1[0] + 1) & 0x3F;

        if (ctx.NRx1[0] & 0x3F == 64) // Disable if hit 64
        {
            ctx.NR52 &= ~(1 << 0);
        }
    }

    if (BIT(ctx.NRx4[1], 6)) // Increment length timer
    {
        ctx.NRx1[1] = ctx.NRx1[1] & 0xC0 + (ctx.NRx1[1] + 1) & 0x3F;

        if (ctx.NRx1[1] & 0x3F == 64) // Disable if hit 64
        {
            ctx.NR52 &= ~(1 << 1);
        }
    }

    if (BIT(ctx.NRx4[2], 6)) // Increment length timer
    {
        ctx.NRx1[2]++;

        if (ctx.NRx1[2] & 0x3F == 256) // Disable if hit 256
        {
            ctx.NR52 &= ~(1 << 2);
        }
    }

    if (BIT(ctx.NRx4[3], 6)) // Increment length timer
    {
        ctx.NRx1[3] = ctx.NRx1[3] & 0xC0 + (ctx.NRx1[3] + 1) & 0x3F;

        if (ctx.NRx1[3] & 0x3F == 64) // Disable if hit 64
        {
            ctx.NR52 &= ~(1 << 3);
        }
    }
}

void apu_write(u16 address, u8 value) {
#if APU_DEBUG == 1
    printf("APU Writing %02X: %02X\n", address, value);
#endif
    if (APU_POWERED_ON)
    {
        switch(address) {
            case 0xFF10:
                //NR10
                ctx.NR10 = value;
                break;

            case 0xFF1A:
                //NR30
                ctx.NR30 = value;
                break;
            
            case 0xFF11:
            case 0xFF16:
            case 0xFF1B:
            case 0xFF20:
            {
                //NRx1
                u16 offset = (address - 0xFF11) / 5;
                ctx.NRx1[offset] = value;
            }
            break;

            case 0xFF12:
            case 0xFF17:
            case 0xFF1C:
            case 0xFF21:
            {
                //NRx2
                u16 offset = (address - 0xFF12) / 5;
                ctx.NRx2[offset] = value;
            }
            break;

            case 0xFF13:
            case 0xFF18:
            case 0xFF1D:
            case 0xFF22:
            {
                //NRx3
                u16 offset = (address - 0xFF13) / 5;
                ctx.NRx3[offset] = value;
            }
            break;

            case 0xFF14:
            case 0xFF19:
            case 0xFF1E:
            case 0xFF23:
            {
                //NRx4
                u16 offset = (address - 0xFF14) / 5;
                ctx.NRx4[offset] = value;
                trigger[offset] = BIT(ctx.NRx4[offset], 7);
            }
            break;
    
            case 0xFF24:
                //NR50
                ctx.NR50 = value;
                break;
    
            case 0xFF25:
                //NR51
                ctx.NR51 = value;
                break;
        }
    }
    else
    {
        printf("APU Powered Off\n");
    }

    if ((address >= 0xFF30) &&
        (address < 0xFF40))
    {
        u8 offset = (u8)(address - 0xFF30);
        ctx.WaveRAM[offset] = value;
    } 
    else if (address == 0xFF26)
    {
        //NR52
        ctx.NR52 = (value & 0xF0) | (ctx.NR52 & 0xF); //Bottom nibble is read only
    }
}

u8 apu_read(u16 address) {
    u8 retVal;

    switch(address) {
        case 0xFF10:
            //NR10
            retVal = ctx.NR10 |= 0x80;
            break;

        case 0xFF1A:
            //NR30
            retVal = ctx.NR30 |= 0x7F;
            break;
        
        case 0xFF11:
        case 0xFF16:
        {
            //NRx1
            u16 offset = (address - 0xFF11) / 5;
            retVal = ctx.NRx1[offset] |= 0x3F;
        }
        break;

        case 0xFF12:
        case 0xFF17:
        case 0xFF21:
        {
            //NRx2
            u16 offset = (address - 0xFF12) / 5;
            retVal = ctx.NRx2[offset];
        }
        break;
        
        case 0xFF1C:
        {
            //NR32
            u16 offset = (address - 0xFF12) / 5;
            retVal = ctx.NRx2[offset] |= 0x9F;
        }
        break;

        case 0xFF22:
        {
            //NRx3
            u16 offset = (address - 0xFF13) / 5;
            retVal = ctx.NRx3[offset];
        }
        break;

        case 0xFF14:
        case 0xFF19:
        case 0xFF1E:
        case 0xFF23:
        {
            //NRx4
            u16 offset = (address - 0xFF14) / 5;
            retVal = ctx.NRx4[offset] |= 0xBF;
        }
        break;

        case 0xFF24:
            //NR50
            retVal = ctx.NR50;
            break;

        case 0xFF25:
            //NR51
            retVal = ctx.NR51;
            break;

        default:
        {
            //Write-only or unimplemented Registers
            retVal = 0xFF;
        }
        break;
    }

    if ((address >= 0xFF30) &&
        (address < 0xFF40))
    {
        u8 offset = (u8)(address - 0xFF30);
        retVal = ctx.WaveRAM[offset];
    }
    else if (address == 0xFF26)
    {
        //NR52
        retVal = ctx.NR52 |= 0x70;
    }
#if APU_DEBUG == 1
    printf("APU Reading %02X: %02X\n", address, retVal);
#endif
    return retVal;
}