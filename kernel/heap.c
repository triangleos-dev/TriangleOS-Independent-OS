#include "heap.h"
#include "memory.h"

#define PAGE_SIZE 4096ULL
#define HEAP_MAGIC 0x545249414E474C45ULL

struct heap_header
{
    unsigned long long magic;
    unsigned long long pages;
    unsigned long long requested_size;
};

void heap_init(void)
{
}

void *kmalloc(unsigned long long size)
{
    if (size == 0)
        return 0;

    unsigned long long total =
        sizeof(struct heap_header) + size;

    unsigned long long pages =
        (total + PAGE_SIZE - 1) /
        PAGE_SIZE;

    struct heap_header *header =
        (struct heap_header *)
        page_alloc_contiguous(pages);

    if (header == 0)
        return 0;

    header->magic = HEAP_MAGIC;
    header->pages = pages;
    header->requested_size = size;

    return (void *)(header + 1);
}

void kfree(void *address)
{
    if (address == 0)
        return;

    struct heap_header *header =
        ((struct heap_header *)address) - 1;

    if (header->magic != HEAP_MAGIC)
        return;

    unsigned long long pages =
        header->pages;

    header->magic = 0;

    page_free_contiguous(
        (void *)header,
        pages
    );
}
