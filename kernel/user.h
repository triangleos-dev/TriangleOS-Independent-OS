#ifndef TRIANGLEOS_USER_H
#define TRIANGLEOS_USER_H

int user_run(const char *name);

int user_validate_range(
    unsigned long long address,
    unsigned long long length
);

int user_copy_string(
    const char *user_string,
    char *kernel_buffer,
    unsigned long capacity
);

int user_process_active(void);

void user_request_terminate(void);

int user_termination_requested(void);

void user_exit_to_kernel(void)
    __attribute__((noreturn));

#endif
