#include "user.h"
#include "memory.h"
#include "console.h"
#include "fs.h"

#define USER_CODE_BASE   0x00400000ULL
#define USER_CODE_SIZE   0x00200000ULL

#define USER_STACK_BASE  0x00800000ULL
#define USER_STACK_SIZE  0x00200000ULL

#define USER_STACK_TOP \
    (USER_STACK_BASE + USER_STACK_SIZE)

#define PAGE_SIZE 4096ULL
#define PAGE_TABLE_ENTRIES 512ULL

#define PAGE_PRESENT 0x001ULL
#define PAGE_WRITE   0x002ULL
#define PAGE_USER    0x004ULL
#define PAGE_HUGE    0x080ULL

extern void user_enter(
    unsigned long long cr3,
    unsigned long long rip,
    unsigned long long rsp
);

extern void user_exit_to_kernel(void)
    __attribute__((noreturn));

unsigned long long user_kernel_cr3 = 0;
unsigned long long user_kernel_saved_rsp = 0;
unsigned long long user_kernel_return = 0;
unsigned long long user_kernel_stack_top = 0;

static void *user_pml4;
static void *user_pdpt;

static void *user_pd[4];

static void *user_code_pt;
static void *user_stack_pt;

static void *user_code_physical;
static void *user_stack_physical;
static void *user_kernel_stack;

static int user_resources_ready = 0;
static volatile int user_active = 0;
static volatile int user_terminate_requested = 0;

/*
 * Flat Ring 3 test executable:
 *
 *   mov eax, SYS_WRITE
 *   lea rdi, [rel message]
 *   syscall
 *
 *   mov eax, SYS_TICKS
 *   syscall
 *
 *   mov eax, SYS_EXIT
 *   syscall
 *
 *   jmp $
 */
static const unsigned char default_user_program[] =
{
    0xB8, 0x01, 0x00, 0x00, 0x00,

    0x48, 0x8D, 0x3D,
    0x0B, 0x00, 0x00, 0x00,

    0x0F, 0x05,

    0xB8, 0x02, 0x00, 0x00, 0x00,

    0x0F, 0x05,

    0xB8, 0x04, 0x00, 0x00, 0x00,

    0x0F, 0x05,

    0xEB, 0xFE,

    'H', 'e', 'l', 'l', 'o',
    ' ', 'f', 'r', 'o', 'm',
    ' ', 'R', 'i', 'n', 'g',
    ' ', '3', '!', '\n', 0
};

static void memory_zero(
    void *address,
    unsigned long long size
)
{
    unsigned char *bytes =
        (unsigned char *)address;

    for (unsigned long long i = 0;
         i < size;
         i++)
    {
        bytes[i] = 0;
    }
}

static void free_if_present(void *address)
{
    if (address != 0)
        page_free(address);
}

static void free_resources(void)
{
    if (user_code_physical != 0)
    {
        page_free_contiguous(
            user_code_physical,
            USER_CODE_SIZE / PAGE_SIZE
        );

        user_code_physical = 0;
    }

    if (user_stack_physical != 0)
    {
        page_free_contiguous(
            user_stack_physical,
            USER_STACK_SIZE / PAGE_SIZE
        );

        user_stack_physical = 0;
    }

    if (user_kernel_stack != 0)
    {
        page_free_contiguous(
            user_kernel_stack,
            8
        );

        user_kernel_stack = 0;
    }

    free_if_present(user_code_pt);
    free_if_present(user_stack_pt);

    user_code_pt = 0;
    user_stack_pt = 0;

    for (unsigned int i = 0;
         i < 4;
         i++)
    {
        free_if_present(user_pd[i]);
        user_pd[i] = 0;
    }

    free_if_present(user_pdpt);
    free_if_present(user_pml4);

    user_pdpt = 0;
    user_pml4 = 0;

    user_kernel_stack_top = 0;
    user_resources_ready = 0;
}

static int allocate_resources(void)
{
    user_code_physical =
        page_alloc_contiguous(
            USER_CODE_SIZE / PAGE_SIZE
        );

    if (user_code_physical == 0)
        goto fail;

    user_stack_physical =
        page_alloc_contiguous(
            USER_STACK_SIZE / PAGE_SIZE
        );

    if (user_stack_physical == 0)
        goto fail;

    user_kernel_stack =
        page_alloc_contiguous(8);

    if (user_kernel_stack == 0)
        goto fail;

    user_pml4 = page_alloc();
    user_pdpt = page_alloc();

    if (user_pml4 == 0 ||
        user_pdpt == 0)
        goto fail;

    for (unsigned int i = 0;
         i < 4;
         i++)
    {
        user_pd[i] = page_alloc();

        if (user_pd[i] == 0)
            goto fail;
    }

    user_code_pt = page_alloc();
    user_stack_pt = page_alloc();

    if (user_code_pt == 0 ||
        user_stack_pt == 0)
        goto fail;

    memory_zero(
        user_code_physical,
        USER_CODE_SIZE
    );

    memory_zero(
        user_stack_physical,
        USER_STACK_SIZE
    );

    memory_zero(
        user_kernel_stack,
        8ULL * PAGE_SIZE
    );

    memory_zero(user_pml4, PAGE_SIZE);
    memory_zero(user_pdpt, PAGE_SIZE);

    for (unsigned int i = 0;
         i < 4;
         i++)
    {
        memory_zero(
            user_pd[i],
            PAGE_SIZE
        );
    }

    memory_zero(
        user_code_pt,
        PAGE_SIZE
    );

    memory_zero(
        user_stack_pt,
        PAGE_SIZE
    );

    user_kernel_stack_top =
        (unsigned long long)user_kernel_stack +
        (8ULL * PAGE_SIZE);

    user_kernel_stack_top &= ~0xFULL;

    user_resources_ready = 1;

    return 0;

fail:

    free_resources();
    return -1;
}

static int build_address_space(void)
{
    unsigned long long *pml4 =
        (unsigned long long *)user_pml4;

    unsigned long long *pdpt =
        (unsigned long long *)user_pdpt;

    for (unsigned int i = 0;
         i < 4;
         i++)
    {
        pdpt[i] =
            (unsigned long long)user_pd[i] |
            PAGE_PRESENT |
            PAGE_WRITE |
            PAGE_USER;
    }

    pml4[0] =
        (unsigned long long)user_pdpt |
        PAGE_PRESENT |
        PAGE_WRITE |
        PAGE_USER;

    /*
     * Identity-map the first 4 GiB.
     *
     * These entries are supervisor-only by
     * default because PAGE_USER is absent.
     */
    for (unsigned int pd_index = 0;
         pd_index < 4;
         pd_index++)
    {
        unsigned long long *pd =
            (unsigned long long *)user_pd[pd_index];

        for (unsigned int entry = 0;
             entry < PAGE_TABLE_ENTRIES;
             entry++)
        {
            unsigned long long physical =
                (
                    ((unsigned long long)pd_index *
                     PAGE_TABLE_ENTRIES) +
                    entry
                ) * 0x200000ULL;

            pd[entry] =
                physical |
                PAGE_PRESENT |
                PAGE_WRITE |
                PAGE_HUGE;
        }
    }

    unsigned long long *pd0 =
        (unsigned long long *)user_pd[0];

    unsigned long long *code_pt =
        (unsigned long long *)user_code_pt;

    unsigned long long *stack_pt =
        (unsigned long long *)user_stack_pt;

    for (unsigned long long i = 0;
         i < PAGE_TABLE_ENTRIES;
         i++)
    {
        unsigned long long code_physical =
            (unsigned long long)user_code_physical +
            i * PAGE_SIZE;

        unsigned long long stack_physical =
            (unsigned long long)user_stack_physical +
            i * PAGE_SIZE;

        code_pt[i] =
            code_physical |
            PAGE_PRESENT |
            PAGE_USER;

        stack_pt[i] =
            stack_physical |
            PAGE_PRESENT |
            PAGE_WRITE |
            PAGE_USER;
    }

    /*
     * 0x00400000 -> PD index 2.
     */
    pd0[2] =
        (unsigned long long)user_code_pt |
        PAGE_PRESENT |
        PAGE_WRITE |
        PAGE_USER;

    /*
     * 0x00800000 -> PD index 4.
     */
    pd0[4] =
        (unsigned long long)user_stack_pt |
        PAGE_PRESENT |
        PAGE_WRITE |
        PAGE_USER;

    return 0;
}

static int install_default_program(void)
{
    unsigned long long size =
        sizeof(default_user_program);

    if (fs_exists("USER.BIN"))
        return 0;

    console_write(
        "User: installing USER.BIN\n"
    );

    if (fs_create("USER.BIN") != 0)
        return -1;

    if (fs_write_at(
            "USER.BIN",
            0,
            default_user_program,
            size) != 0)
        return -1;

    return 0;
}

int user_validate_range(
    unsigned long long address,
    unsigned long long length
)
{
    if (length == 0)
        return 1;

    if (length > USER_CODE_SIZE &&
        length > USER_STACK_SIZE)
        return 0;

    if (address >= USER_CODE_BASE &&
        address < USER_CODE_BASE + USER_CODE_SIZE &&
        length <=
            USER_CODE_BASE +
            USER_CODE_SIZE -
            address)
        return 1;

    if (address >= USER_STACK_BASE &&
        address < USER_STACK_BASE + USER_STACK_SIZE &&
        length <=
            USER_STACK_BASE +
            USER_STACK_SIZE -
            address)
        return 1;

    return 0;
}

int user_copy_string(
    const char *user_string,
    char *kernel_buffer,
    unsigned long capacity
)
{
    if (user_string == 0 ||
        kernel_buffer == 0 ||
        capacity < 2)
        return -1;

    for (unsigned long i = 0;
         i < capacity - 1;
         i++)
    {
        unsigned long long address =
            (unsigned long long)user_string + i;

        if (!user_validate_range(
                address,
                1))
            return -1;

        char value =
            user_string[i];

        if (value == '\0')
        {
            kernel_buffer[i] = '\0';
            return (int)i;
        }

        kernel_buffer[i] = value;
    }

    kernel_buffer[capacity - 1] = '\0';

    return -1;
}

int user_process_active(void)
{
    return user_active;
}

void user_request_terminate(void)
{
    if (user_active)
        user_terminate_requested = 1;
}

int user_termination_requested(void)
{
    return user_terminate_requested;
}

int user_run(const char *name)
{
    if (name == 0 ||
        *name == '\0')
        return -1;

    if (!fs_exists(name))
    {
        if (name[0] == 'U' &&
            name[1] == 'S' &&
            name[2] == 'E' &&
            name[3] == 'R' &&
            name[4] == '.' &&
            name[5] == 'B' &&
            name[6] == 'I' &&
            name[7] == 'N' &&
            name[8] == '\0')
        {
            if (install_default_program() != 0)
            {
                console_write(
                    "User: installation failed\n"
                );

                return -1;
            }
        }
        else
        {
            console_write(
                "User: executable not found\n"
            );

            return -1;
        }
    }

    unsigned long size;

    if (fs_get_size(
            name,
            &size) != 0)
    {
        console_write(
            "User: cannot read executable size\n"
        );

        return -1;
    }

    if (size == 0 ||
        size > USER_CODE_SIZE)
    {
        console_write(
            "User: executable too large or empty\n"
        );

        return -1;
    }

    if (allocate_resources() != 0)
    {
        console_write(
            "User: memory allocation failed\n"
        );

        return -1;
    }

    if (fs_load(
            name,
            user_code_physical,
            USER_CODE_SIZE,
            &size) != 0)
    {
        console_write(
            "User: executable load failed\n"
        );

        free_resources();
        return -1;
    }

    if (build_address_space() != 0)
    {
        free_resources();
        return -1;
    }

    user_terminate_requested = 0;
    user_active = 1;

    console_write("User: loading ");
    console_write(name);
    console_putc('\n');

    console_write(
        "User: entering Ring 3\n"
    );

    user_enter(
        (unsigned long long)user_pml4,
        USER_CODE_BASE,
        USER_STACK_TOP
    );

    user_active = 0;
    user_terminate_requested = 0;

    console_write(
        "User: process exited\n"
    );

    free_resources();

    return 0;
}
