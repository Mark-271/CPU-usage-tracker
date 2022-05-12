// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Author: Sam Protsenko <joe.skb7@gmail.com>
 */

#include <tools.h>
#include <common.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

#define EINTR_LOOP(var, cmd)			\
	do {					\
		var = cmd;			\
	} while (var == -1 && errno == EINTR)

/**
 * Sleep for interval specified in milli-seconds.
 *
 * @param msec Milli-seconds to sleep; can be greater than a second
 */
void msleep(unsigned long msec)
{
	int r;
	struct timespec wait;

	errno = 0;
	wait.tv_sec = msec / 1000;
	wait.tv_nsec = (msec % 1000) * 1e6;

	EINTR_LOOP(r, nanosleep(&wait, NULL));
}

void clear_screen(void)
{
	const char *clear_screen_ansi = CLEAR_SCREEN;
	int r;

	EINTR_LOOP(r, write(STDOUT_FILENO, clear_screen_ansi, 11));
}
