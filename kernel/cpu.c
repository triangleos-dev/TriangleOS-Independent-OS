#include "cpu.h"

struct cpuid_result
{
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
};

static struct cpu_info info;


static struct cpuid_result cpuid(
    unsigned int leaf
)
{
    struct cpuid_result result;

    __asm__ volatile (
        "cpuid"
        : "=a"(result.eax),
          "=b"(result.ebx),
          "=c"(result.ecx),
          "=d"(result.edx)
        : "a"(leaf)
    );

    return result;
}


static struct cpuid_result cpuid_extended(
    unsigned int leaf
)
{
    struct cpuid_result result;

    __asm__ volatile (
        "cpuid"
        : "=a"(result.eax),
          "=b"(result.ebx),
          "=c"(result.ecx),
          "=d"(result.edx)
        : "a"(leaf)
    );

    return result;
}


void cpu_init(void)
{
    struct cpuid_result basic =
        cpuid(0);

    /*
     * Vendor string:
     *
     * EBX ECX EDX
     */
    ((unsigned int *)&info.vendor[0])[0] =
        basic.ebx;

    ((unsigned int *)&info.vendor[0])[1] =
        basic.edx;

    ((unsigned int *)&info.vendor[0])[2] =
        basic.ecx;

    info.vendor[12] = '\0';


    /*
     * Basic feature information.
     */
    struct cpuid_result leaf1 =
        cpuid(1);


    info.stepping =
        leaf1.eax & 0xF;


    unsigned int base_model =
        (leaf1.eax >> 4) & 0xF;

    unsigned int base_family =
        (leaf1.eax >> 8) & 0xF;

    unsigned int ext_model =
        (leaf1.eax >> 16) & 0xF;

    unsigned int ext_family =
        (leaf1.eax >> 20) & 0xFF;


    /*
     * Intel/AMD architectural family/model calculation.
     */
    info.family = base_family;

    if (base_family == 0xF)
        info.family += ext_family;


    info.model = base_model;

    if (base_family == 0x6 ||
        base_family == 0xF)
    {
        info.model |=
            ext_model << 4;
    }


    /*
     * EBX[23:16]
     *
     * Maximum logical processor IDs
     * addressable by this CPUID leaf.
     */
    info.logical_processors =
        (leaf1.ebx >> 16) & 0xFF;


    /*
     * EDX feature bits.
     */
    info.apic =
        (leaf1.edx >> 9) & 1;

    info.sse =
        (leaf1.edx >> 25) & 1;

    info.sse2 =
        (leaf1.edx >> 26) & 1;


    /*
     * Extended CPUID.
     */
    struct cpuid_result maximum =
        cpuid_extended(0x80000000);


    info.long_mode = 0;

    if (maximum.eax >= 0x80000001)
    {
        struct cpuid_result ext =
            cpuid_extended(0x80000001);

        /*
         * EDX bit 29 = long mode.
         */
        info.long_mode =
            (ext.edx >> 29) & 1;
    }
}


const struct cpu_info *cpu_get_info(void)
{
    return &info;
}
