/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Author: Mark Sungurov <mark.sungurov@gmail.com>
 */

#ifndef CPU_MONITOR_H
#define CPU_MONITOR_H

#define CPU_SAMPLING_DELAY	200 /* msec */

int cpu_monitor_init(void);
void cpu_monitor_exit(void);
int cpu_monitor_read_data(void);
void cpu_monitor_analyze_data(void);
void cpu_monitor_print_res(void);

#endif /* CPU_MONITOR_H */
