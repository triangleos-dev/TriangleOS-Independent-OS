#ifndef TRIANGLEOS_MEMORY_H
#define TRIANGLEOS_MEMORY_H

void memory_init(void);
void memory_print_info(void);

void *page_alloc(void);
void *page_alloc_contiguous(unsigned long long pages);

void page_free(void *address);
void page_free_contiguous(
    void *address,
    unsigned long long pages
);

unsigned long long memory_free_pages(void);

#endif
