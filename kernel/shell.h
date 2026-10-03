#ifndef TRIANGLEOS_SHELL_H
#define TRIANGLEOS_SHELL_H

void shell_init(void);

void shell_input(char c);
void shell_backspace(void);
void shell_enter(void);
void shell_cancel(void);

void shell_history_up(void);
void shell_history_down(void);

#endif
