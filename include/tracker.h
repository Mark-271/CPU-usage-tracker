/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Author: Mark Sungurov <mark.sungurov@gmail.com>
 */

#ifndef TRACKER_H
#define TRACKER_H

#define THR_NUM		3

int tracker_init(void);
void tracker_exit(void);
void *tracker_read(void *data);
void *tracker_analyze(void *data);
void *tracker_print(void *data);
void tracker_sig_handler(int signum);

#endif /* TRACKER_H */
