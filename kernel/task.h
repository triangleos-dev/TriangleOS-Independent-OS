#ifndef TRIANGLEOS_TASK_H
#define TRIANGLEOS_TASK_H

#define TASK_MAX 16

enum task_state
{
    TASK_UNUSED,
    TASK_READY,
    TASK_RUNNING,
    TASK_DEAD
};

struct task_context
{
    unsigned long long rsp;
    unsigned long long rbx;
    unsigned long long rbp;
    unsigned long long r12;
    unsigned long long r13;
    unsigned long long r14;
    unsigned long long r15;
};

struct task
{
    unsigned int pid;
    enum task_state state;

    void (*entry)(void *arg);
    void *arg;

    void *stack;

    struct task_context context;
};

void task_init(void);

int task_create(
    void (*entry)(void *arg),
    void *arg
);

void task_yield(void);
void task_exit(void);

void task_list(void);
void task_spawn_demo(void);

#endif
