#include "memory.h"
#include "console.h"

#define PAGE_SIZE 4096ULL

#define E820_COUNT_ADDRESS 0x4FF0ULL
#define E820_MAP_ADDRESS   0x5000ULL

#define BITMAP_ADDRESS 0x60000ULL

#define MAX_PHYSICAL_MEMORY 0x100000000ULL
#define MAX_FRAMES \
    (MAX_PHYSICAL_MEMORY / PAGE_SIZE)

#define BITMAP_SIZE \
    (MAX_FRAMES / 8ULL)

#define LOW_MEMORY_END 0x100000ULL

#define STAGE2_START 0x8000ULL
#define STAGE2_END   0x10000ULL

#define PAGE_TABLE_START 0x9000ULL
#define PAGE_TABLE_END   0xF000ULL

#define KERNEL_LOAD_BUFFER 0x20000ULL
#define KERNEL_LOAD_END    0x28000ULL

#define KERNEL_STACK_START 0x80000ULL
#define KERNEL_STACK_END   0xA0000ULL

struct e820_entry
{
    unsigned long long base;
    unsigned long long length;
    unsigned int type;
    unsigned int attributes;
} __attribute__((packed));

static volatile unsigned char *bitmap =
    (volatile unsigned char *)BITMAP_ADDRESS;

static unsigned long long free_pages;

extern char __kernel_start;
extern char __kernel_end;

static void bitmap_fill(void)
{
    for (unsigned long long i = 0;
         i < BITMAP_SIZE;
         i++)
    {
        bitmap[i] = 0xFF;
    }
}

static void bitmap_set(unsigned long long frame)
{
    bitmap[frame >> 3] |=
        (unsigned char)(1ULL << (frame & 7));
}

static void bitmap_clear(unsigned long long frame)
{
    bitmap[frame >> 3] &=
        (unsigned char)~(1ULL << (frame & 7));
}

static int bitmap_test(unsigned long long frame)
{
    return bitmap[frame >> 3] &
           (unsigned char)(1ULL << (frame & 7));
}

static unsigned long long align_up(
    unsigned long long value)
{
    return (value + PAGE_SIZE - 1) &
           ~(PAGE_SIZE - 1);
}

static unsigned long long align_down(
    unsigned long long value)
{
    return value & ~(PAGE_SIZE - 1);
}

static void mark_free(
    unsigned long long base,
    unsigned long long length)
{
    if (base >= MAX_PHYSICAL_MEMORY)
        return;

    unsigned long long end;

    if (length > MAX_PHYSICAL_MEMORY - base)
        end = MAX_PHYSICAL_MEMORY;
    else
        end = base + length;

    unsigned long long start =
        align_up(base);

    end = align_down(end);

    if (start >= end)
        return;

    unsigned long long first =
        start / PAGE_SIZE;

    unsigned long long last =
        end / PAGE_SIZE;

    for (unsigned long long frame = first;
         frame < last;
         frame++)
    {
        bitmap_clear(frame);
    }
}

static void mark_reserved(
    unsigned long long start,
    unsigned long long end)
{
    if (start >= MAX_PHYSICAL_MEMORY)
        return;

    if (end > MAX_PHYSICAL_MEMORY)
        end = MAX_PHYSICAL_MEMORY;

    start = align_down(start);
    end = align_up(end);

    if (start >= end)
        return;

    unsigned long long first =
        start / PAGE_SIZE;

    unsigned long long last =
        end / PAGE_SIZE;

    for (unsigned long long frame = first;
         frame < last;
         frame++)
    {
        bitmap_set(frame);
    }
}

static void count_free_pages(void)
{
    free_pages = 0;

    for (unsigned long long frame = 0;
         frame < MAX_FRAMES;
         frame++)
    {
        if (!bitmap_test(frame))
            free_pages++;
    }
}

void memory_init(void)
{
    bitmap_fill();

    volatile unsigned short *count =
        (volatile unsigned short *)
        E820_COUNT_ADDRESS;

    volatile struct e820_entry *map =
        (volatile struct e820_entry *)
        E820_MAP_ADDRESS;

    unsigned int entries = *count;

    for (unsigned int i = 0;
         i < entries && i < 128;
         i++)
    {
        if (map[i].type == 1)
        {
            mark_free(
                map[i].base,
                map[i].length
            );
        }
    }

    mark_reserved(0, LOW_MEMORY_END);

    mark_reserved(
        STAGE2_START,
        STAGE2_END
    );

    mark_reserved(
        PAGE_TABLE_START,
        PAGE_TABLE_END
    );

    mark_reserved(
        KERNEL_LOAD_BUFFER,
        KERNEL_LOAD_END
    );

    mark_reserved(
        (unsigned long long)&__kernel_start,
        (unsigned long long)&__kernel_end
    );

    mark_reserved(
        KERNEL_STACK_START,
        KERNEL_STACK_END
    );

    mark_reserved(
        BITMAP_ADDRESS,
        BITMAP_ADDRESS + BITMAP_SIZE
    );

    count_free_pages();
}

void memory_print_info(void)
{
    volatile unsigned short *count =
        (volatile unsigned short *)
        E820_COUNT_ADDRESS;

    console_write("E820 entries: ");
    console_write_uint(*count);
    console_write("\n");

    console_write("Free pages: ");
    console_write_uint(free_pages);
    console_write("\n");

    console_write(
        "Physical memory limit: 4 GiB\n"
    );
}

unsigned long long memory_free_pages(void)
{
    return free_pages;
}

void *page_alloc(void)
{
    for (unsigned long long frame = 256;
         frame < MAX_FRAMES;
         frame++)
    {
        if (!bitmap_test(frame))
        {
            bitmap_set(frame);

            if (free_pages > 0)
                free_pages--;

            return (void *)
                (frame * PAGE_SIZE);
        }
    }

    return 0;
}

void *page_alloc_contiguous(
    unsigned long long pages)
{
    if (pages == 0)
        return 0;

    unsigned long long run = 0;

    for (unsigned long long frame = 256;
         frame < MAX_FRAMES;
         frame++)
    {
        if (!bitmap_test(frame))
            run++;
        else
            run = 0;

        if (run == pages)
        {
            unsigned long long first =
                frame + 1 - pages;

            for (unsigned long long i = 0;
                 i < pages;
                 i++)
            {
                bitmap_set(first + i);
            }

            if (free_pages >= pages)
                free_pages -= pages;
            else
                free_pages = 0;

            return (void *)
                (first * PAGE_SIZE);
        }
    }

    return 0;
}

void page_free(void *address)
{
    unsigned long long value =
        (unsigned long long)address;

    if (value == 0)
        return;

    if (value >= MAX_PHYSICAL_MEMORY)
        return;

    if (value & (PAGE_SIZE - 1))
        return;

    unsigned long long frame =
        value / PAGE_SIZE;

    if (bitmap_test(frame))
    {
        bitmap_clear(frame);
        free_pages++;
    }
}

void page_free_contiguous(
    void *address,
    unsigned long long pages)
{
    if (address == 0 || pages == 0)
        return;

    unsigned long long value =
        (unsigned long long)address;

    if (value & (PAGE_SIZE - 1))
        return;

    if (value >= MAX_PHYSICAL_MEMORY)
        return;

    unsigned long long first =
        value / PAGE_SIZE;

    if (pages > MAX_FRAMES - first)
        return;

    for (unsigned long long i = 0;
         i < pages;
         i++)
    {
        if (bitmap_test(first + i))
        {
            bitmap_clear(first + i);
            free_pages++;
        }
    }
}
