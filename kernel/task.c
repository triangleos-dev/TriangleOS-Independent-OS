#include "task.h"
#include "memory.h"
#include "console.h"

#define TASK_STACK_PAGES 4

extern void task_switch(
    struct task_context *old,
    struct task_context *new
);

static struct task tasks[TASK_MAX];

static unsigned int current_task = 0;
static unsigned int next_pid = 1;


static struct task *current(void)
{
    return &tasks[current_task];
}


static int find_ready_task(void)
{
    for (unsigned int offset = 1;
         offset <= TASK_MAX;
         offset++)
    {
        unsigned int index =
            (current_task + offset) % TASK_MAX;

        if (tasks[index].state == TASK_READY)
            return (int)index;
    }

    return -1;
}


static void task_trampoline(void)
{
    struct task *task = current();

    task->entry(task->arg);

    task_exit();

    for (;;)
    {
        __asm__ volatile ("hlt");
    }
}


void task_init(void)
{
    for (unsigned int i = 0;
         i < TASK_MAX;
         i++)
    {
        tasks[i].pid = 0;
        tasks[i].state = TASK_UNUSED;
        tasks[i].entry = 0;
        tasks[i].arg = 0;
        tasks[i].stack = 0;

        tasks[i].context.rsp = 0;
        tasks[i].context.rbx = 0;
        tasks[i].context.rbp = 0;
        tasks[i].context.r12 = 0;
        tasks[i].context.r13 = 0;
        tasks[i].context.r14 = 0;
        tasks[i].context.r15 = 0;
    }

    /*
     * PID 0 represents the original kernel execution.
     */
    tasks[0].pid = 0;
    tasks[0].state = TASK_RUNNING;

    current_task = 0;
    next_pid = 1;
}


int task_create(
    void (*entry)(void *arg),
    void *arg
)
{
    if (entry == 0)
        return -1;

    int slot = -1;

    for (unsigned int i = 1;
         i < TASK_MAX;
         i++)
    {
        if (tasks[i].state == TASK_UNUSED ||
            tasks[i].state == TASK_DEAD)
        {
            slot = (int)i;
            break;
        }
    }

    if (slot < 0)
        return -1;


    void *stack =
        page_alloc_contiguous(
            TASK_STACK_PAGES
        );

    if (stack == 0)
        return -1;


    unsigned long long stack_top =
        (unsigned long long)stack +
        (TASK_STACK_PAGES * 4096ULL);

    /*
     * Keep the stack aligned.
     */
    stack_top &= ~0xFULL;


    /*
     * task_switch() expects the new stack to contain:
     *
     *   +0   r15
     *   +8   r14
     *   +16  r13
     *   +24  r12
     *   +32  rbp
     *   +40  rbx
     *   +48  return RIP
     *
     * task_switch() restores those registers and then
     * executes RET.
     */
    unsigned long long *stack_frame =
        (unsigned long long *)
        (stack_top - 7 * sizeof(unsigned long long));


    stack_frame[0] = 0;  /* r15 */
    stack_frame[1] = 0;  /* r14 */
    stack_frame[2] = 0;  /* r13 */
    stack_frame[3] = 0;  /* r12 */
    stack_frame[4] = 0;  /* rbp */
    stack_frame[5] = 0;  /* rbx */

    /*
     * First instruction executed by the new task.
     */
    stack_frame[6] =
        (unsigned long long)task_trampoline;


    tasks[slot].pid = next_pid++;
    tasks[slot].state = TASK_READY;
    tasks[slot].entry = entry;
    tasks[slot].arg = arg;
    tasks[slot].stack = stack;

    tasks[slot].context.rsp =
        (unsigned long long)stack_frame;

    tasks[slot].context.rbx = 0;
    tasks[slot].context.rbp = 0;
    tasks[slot].context.r12 = 0;
    tasks[slot].context.r13 = 0;
    tasks[slot].context.r14 = 0;
    tasks[slot].context.r15 = 0;

    return (int)tasks[slot].pid;
}


/*
 * Simple demonstration task used by the shell's "spawn"
 * command.
 */
static void demo_task(void *arg)
{
    (void)arg;

    for (;;)
    {
        console_write("demo task running\n");

        /*
         * Small delay so the console isn't flooded.
         */
        for (volatile unsigned long i = 0;
             i < 1000000UL;
             i++)
        {
            __asm__ volatile ("pause");
        }

        task_yield();
    }
}


void task_spawn_demo(void)
{
    int pid = task_create(
        demo_task,
        0
    );

    if (pid < 0)
    {
        console_write("spawn: failed\n");
        return;
    }

    console_write("spawned PID ");

    console_write_uint(
        (unsigned int)pid
    );

    console_write("\n");
}


void task_yield(void)
{
    int next = find_ready_task();

    if (next < 0)
        return;


    unsigned int old =
        current_task;

    unsigned int new_task =
        (unsigned int)next;


    tasks[old].state = TASK_READY;
    tasks[new_task].state = TASK_RUNNING;

    current_task = new_task;


    task_switch(
        &tasks[old].context,
        &tasks[new_task].context
    );


    /*
     * Execution reaches here when the old
     * task is scheduled again.
     */
    current_task = old;
    tasks[old].state = TASK_RUNNING;
}


void task_exit(void)
{
    struct task *task = current();

    unsigned int old =
        current_task;


    /*
     * Do NOT free the current task's stack here.
     *
     * We are still executing on it.
     */
    task->state = TASK_DEAD;


    int next = find_ready_task();

    if (next < 0)
    {
        /*
         * Nothing else exists.
         */
        for (;;)
        {
            __asm__ volatile ("hlt");
        }
    }


    unsigned int new_task =
        (unsigned int)next;

    tasks[new_task].state =
        TASK_RUNNING;

    current_task = new_task;


    task_switch(
        &tasks[old].context,
        &tasks[new_task].context
    );


    /*
     * An exited task must never resume.
     */
    for (;;)
    {
        __asm__ volatile ("hlt");
    }
}


void task_list(void)
{
    console_write("PID  STATE\n");


    for (unsigned int i = 0;
         i < TASK_MAX;
         i++)
    {
        if (tasks[i].state == TASK_UNUSED)
            continue;


        console_write_uint(
            tasks[i].pid
        );

        console_write("    ");


        switch (tasks[i].state)
        {
            case TASK_READY:
                console_write("READY");
                break;

            case TASK_RUNNING:
                console_write("RUNNING");
                break;

            case TASK_DEAD:
                console_write("DEAD");
                break;

            default:
                console_write("UNKNOWN");
                break;
        }

        console_write("\n");
    }
}
