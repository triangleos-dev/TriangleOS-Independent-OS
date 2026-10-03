#include "idt.h"

struct idt_entry
{
    unsigned short offset_low;
    unsigned short selector;
    unsigned char  ist;
    unsigned char  attributes;
    unsigned short offset_middle;
    unsigned int   offset_high;
    unsigned int   reserved;
} __attribute__((packed));

struct idt_ptr
{
    unsigned short limit;
    unsigned long base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr idtr;

extern void default_isr(void);
extern void timer_isr(void);
extern void keyboard_isr(void);
extern void syscall_isr(void);


static void idt_set_gate(
    int vector,
    unsigned long handler
)
{
    idt[vector].offset_low =
        handler & 0xFFFF;

    idt[vector].selector = 0x18;
    idt[vector].ist = 0;
    idt[vector].attributes = 0x8E;

    idt[vector].offset_middle =
        (handler >> 16) & 0xFFFF;

    idt[vector].offset_high =
        (handler >> 32) & 0xFFFFFFFF;

    idt[vector].reserved = 0;
}


void idt_init(void)
{
    for (int i = 0; i < 256; i++)
    {
        idt_set_gate(
            i,
            (unsigned long)default_isr
        );
    }

    /* IRQ0: PIT */
    idt_set_gate(
        32,
        (unsigned long)timer_isr
    );

    /* IRQ1: keyboard */
    idt_set_gate(
        33,
        (unsigned long)keyboard_isr
    );

    /* Software syscall interrupt */
    idt_set_gate(
        0x80,
        (unsigned long)syscall_isr
    );

    idtr.limit =
        sizeof(idt) - 1;

    idtr.base =
        (unsigned long)&idt;

    __asm__ volatile (
        "lidt %0"
        :
        : "m"(idtr)
    );
}
