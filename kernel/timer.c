#include "timer.h"
#include "io.h"

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43

#define PIT_BASE_FREQUENCY 1193182U
#define PIT_FREQUENCY      100U

volatile unsigned long long timer_ticks = 0;

void timer_init(void)
{
    unsigned short divisor =
        PIT_BASE_FREQUENCY / PIT_FREQUENCY;

    /*
     * Channel 0
     * Access mode: low byte, then high byte
     * Mode 3: square wave
     * Binary mode
     */
    outb(PIT_COMMAND, 0x36);

    outb(
        PIT_CHANNEL0,
        divisor & 0xFF
    );

    outb(
        PIT_CHANNEL0,
        (divisor >> 8) & 0xFF
    );

    timer_ticks = 0;
}

unsigned long long timer_get_ticks(void)
{
    return timer_ticks;
}
