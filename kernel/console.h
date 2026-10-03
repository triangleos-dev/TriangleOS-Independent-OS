#ifndef TRIANGLEOS_CONSOLE_H
#define TRIANGLEOS_CONSOLE_H

void console_init(void);
void console_clear(void);

void console_putc(char c);
void console_write(const char *text);

void console_write_uint(unsigned long long value);

void console_backspace(void);

#endif
