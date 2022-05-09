// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Author: Mark Sungurov <mark.sungurov@gmail.com>
 */

#include <cpu_monitor.h>
#include <common.h>
#include <pthread.h>
#include <tools.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>

typedef void *(*thread_func_t) (void *);

int main(void)
{
	int err, ret = EXIT_SUCCESS;
	size_t i;
	pthread_t thr_ids[THR_NUM];

	thread_func_t thread_func[] = {
		thread_reader_func,
		thread_analyzer_func,
		thread_printer_func
	};

	if (signal(SIGINT, sig_handler) == SIG_ERR)
		fprintf(stderr, "Warning: Can't catch SIGINT\n");

	err = cpu_monitor_init();
	if (err) {
		ret = EXIT_FAILURE;
		goto exit;
	}

	for (i = 0; i < THR_NUM; i++) {
		err = pthread_create(&thr_ids[i], NULL,
				     thread_func[i], NULL);
		if (err) {
			perror("Warning: Error in pthread_create()");
			ret = EXIT_FAILURE;
			goto exit;
		}
	}

	for (i = 0; i < THR_NUM; i++) {
		err = pthread_join(thr_ids[i], NULL);
		if (err) {
			fprintf(stderr, "Error: pthread_join(), err=%d\n", err);
			ret = EXIT_FAILURE;
			goto exit;
		}
	}

exit:
	cpu_monitor_exit();
	return ret;
}
