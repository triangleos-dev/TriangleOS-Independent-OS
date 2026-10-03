#ifndef TRIANGLEOS_CPU_H
#define TRIANGLEOS_CPU_H

struct cpu_info
{
    char vendor[13];

    unsigned int family;
    unsigned int model;
    unsigned int stepping;

    unsigned int logical_processors;

    int long_mode;
    int sse;
    int sse2;
    int apic;
};

void cpu_init(void);
const struct cpu_info *cpu_get_info(void);

#endif
