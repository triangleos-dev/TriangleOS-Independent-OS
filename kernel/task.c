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

    stack_top &= ~0xFULL;

    /*
     * task_switch ends with RET.
     * Put task_trampoline where RET will find it.
     */
    unsigned long long *return_address =
        (unsigned long long *)(stack_top - 8);

    *return_address =
        (unsigned long long)task_trampoline;


    tasks[slot].pid = next_pid++;
    tasks[slot].state = TASK_READY;
    tasks[slot].entry = entry;
    tasks[slot].arg = arg;
    tasks[slot].stack = stack;

    tasks[slot].context.rsp =
        stack_top - 8;

    tasks[slot].context.rbx = 0;
    tasks[slot].context.rbp = 0;
    tasks[slot].context.r12 = 0;
    tasks[slot].context.r13 = 0;
    tasks[slot].context.r14 = 0;
    tasks[slot].context.r15 = 0;

    return (int)tasks[slot].pid;
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

    task->state = TASK_DEAD;

    if (task->stack != 0)
    {
        page_free_contiguous(
            task->stack,
            TASK_STACK_PAGES
        );

        task->stack = 0;
    }

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

        console_putc('\n');
    }
}


static void demo_task(void *arg)
{
    (void)arg;

    for (int i = 1; i <= 3; i++)
    {
        console_write("Demo task iteration: ");
        console_write_uint(i);
        console_putc('\n');

        task_yield();
    }
}


void task_spawn_demo(void)
{
    int pid =
        task_create(demo_task, 0);

    if (pid < 0)
    {
        console_write(
            "Failed to create task.\n"
        );

        return;
    }

    console_write("Created task PID ");
    console_write_uint((unsigned int)pid);
    console_putc('\n');
}
