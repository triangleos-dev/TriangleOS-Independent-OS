#include "pic.h"
#include "io.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21

#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define ICW1_INIT    0x10
#define ICW1_ICW4    0x01
#define ICW4_8086    0x01

static void io_wait(void)
{
    outb(0x80, 0);
}

void pic_init(void)
{
    /*
     * Initialize both PICs.
     */
    outb(
        PIC1_COMMAND,
        ICW1_INIT | ICW1_ICW4
    );

    io_wait();

    outb(
        PIC2_COMMAND,
        ICW1_INIT | ICW1_ICW4
    );

    io_wait();

    /*
     * Remap master to 0x20.
     * Remap slave to 0x28.
     */
    outb(PIC1_DATA, 0x20);
    io_wait();

    outb(PIC2_DATA, 0x28);
    io_wait();

    /*
     * Slave connected to master IRQ2.
     */
    outb(PIC1_DATA, 0x04);
    io_wait();

    outb(PIC2_DATA, 0x02);
    io_wait();

    /*
     * 8086 mode.
     */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();

    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /*
     * Enable:
     *
     * IRQ0 = timer
     * IRQ1 = keyboard
     *
     * Everything else disabled.
     */
    outb(PIC1_DATA, 0xFC);

    /*
     * Disable all slave IRQs for now.
     */
    outb(PIC2_DATA, 0xFF);
}
