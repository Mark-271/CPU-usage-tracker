#ifndef CPU_MONITOR_H
#define CPU_MONITOR_H

#include <stddef.h>

/* CPU core usage data */
struct  core_stat {
	char name[5];
	unsigned long user;
	unsigned long nice;
	unsigned long system;
	unsigned long idle;
	unsigned long iowait;
	unsigned long irq;
	unsigned long softirq;
	unsigned long steal;
};

struct cpu_stat {
	/* The number of cpu cores both physical and virtual */
	size_t cpu_num;
	struct core_stat *cs;
};

extern struct cpu_stat st;

int cpu_monitor_read_data(const char *path);

#endif /* CPU_MONITOR_H */
