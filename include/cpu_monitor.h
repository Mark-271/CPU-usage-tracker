/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Author: Mark Sungurov <mark.sungurov@gmail.com>
 */

#ifndef CPU_MONITOR_H
#define CPU_MONITOR_H

#define THR_NUM		3

int cpu_monitor_init(void);
void cpu_monitor_exit(void);
void *thread_reader_func(void *data);
void *thread_analyzer_func(void *data);
void *thread_printer_func(void *data);
void sig_handler(int signum);

#endif /* CPU_MONITOR_H */
