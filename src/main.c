// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Author: Mark Sungurov <mark.sungurov@gmail.com>
 */

#include <cpu_monitor.h>
#include <file.h>
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

#define THR_NUM		3
#define EXIT_SIGNAL	128 /* terminated by signal */

static pthread_t thr_ids[THR_NUM];
static pthread_cond_t cond_ids[THR_NUM - 1];
static pthread_mutex_t condlock;
static bool thr_cancel; /* Flag indicating that all threads should exit */

/* Set mask for blocked signals */
static void thread_sig_mask(void)
{
	int err;
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGINT);

	err = pthread_sigmask(SIG_BLOCK, &set, NULL);
	if (err != 0)
		printf("Warning: Can't unable sigmask\n");
}

static void *thread_reader_func(void *data)
{
	UNUSED(data);
	thread_sig_mask();

	for (;;) {
		int err;
		if (thr_cancel) {
			pthread_cond_signal(&cond_ids[0]);
			break;
		}

		err = cpu_monitor_read_data();
		if (err) {
			printf("Error: thread_reader error\n");
			break;
		}
		pthread_cond_signal(&cond_ids[0]);

		msleep(CPU_SAMPLING_DELAY);
	}

	return NULL;
}

static void *thread_analyzer_func(void *data)
{
	UNUSED(data);
	thread_sig_mask();

	for (;;) {
		pthread_mutex_lock(&condlock);
		pthread_cond_wait(&cond_ids[0], &condlock);
		pthread_mutex_unlock(&condlock);

		if (thr_cancel) {
			pthread_cond_signal(&cond_ids[1]);
			break;
		}

		cpu_monitor_analyze_data();
		pthread_cond_signal(&cond_ids[1]);
	}

	return NULL;
}

static void *thread_printer_func(void *data)
{
	UNUSED(data);
	thread_sig_mask();

	for (;;) {
		pthread_mutex_lock(&condlock);
		pthread_cond_wait(&cond_ids[1], &condlock);
		pthread_mutex_unlock(&condlock);

		if (thr_cancel)
			break;

		cpu_monitor_print_res();
	}

	return NULL;
}

static void destroy_cond(void)
{
	for(size_t i = 0; i < THR_NUM - 1; ++i)
		pthread_cond_destroy(&cond_ids[i]);
	pthread_mutex_destroy(&condlock);
}

/*
 * When sig_handler is invoked, it sets the thr_cancel flag,
 * forcing the pthread threads termination, which then are joint by main thread.
 */
static void sig_handler(int signum)
{
	if (signum == SIGINT) {
		thr_cancel = true;
		printf("Bye!\n");
	}
}

static thread_func_t thread_func[] = {
					thread_reader_func,
					thread_analyzer_func,
					thread_printer_func
};

int main(void)
{
	int err, ret = EXIT_SUCCESS;
	size_t i;

	if (signal(SIGINT, sig_handler) == SIG_ERR)
		fprintf(stderr, "Warning: Can't catch SIGINT\n");

	pthread_mutex_init(&condlock, NULL);

	for (i = 0; i < THR_NUM - 1; ++i)
		pthread_cond_init(&cond_ids[i], NULL);

	err = cpu_monitor_init();
	if (err) {
		err = EXIT_FAILURE;
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
	destroy_cond();
	cpu_monitor_exit();
	return ret;
}
