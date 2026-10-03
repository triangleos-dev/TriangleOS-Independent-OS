#ifndef TRIANGLEOS_HEAP_H
#define TRIANGLEOS_HEAP_H

void heap_init(void);

void *kmalloc(unsigned long long size);
void kfree(void *address);

#endif
