#include <cpu_monitor.h>
#include <file.h>
#include <common.h>
#include <tools.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <threads.h>

typedef void *(*thread_func_t) (void *);

#define EXIT_SIGNAL	128	/* terminated by signal */

pthread_t thr_ids[THR_NUM];
pthread_cond_t cond_ids[THR_NUM - 1];
pthread_mutex_t lock;
pthread_mutex_t condlock;

static void *thread_reader_func(void *data)
{
	UNUSED(data);
	int err;
	/* TODO: function to block some signals */
	for (;;) {
		err = cpu_monitor_read_data();
		if (err) {
			printf("Error: thread_reader error\n");
			break;
		}
		pthread_cond_signal(&cond_ids[0]);

		msleep(THR_READER_DELAY);
	}

	return NULL;
}

static void *thread_analyzer_func(void *data)
{
	UNUSED(data);
	/* TODO: function to block some signals */
	for (;;) {
		pthread_mutex_lock(&condlock);
		pthread_cond_wait(&cond_ids[0], &condlock);
		pthread_mutex_unlock(&condlock);

		cpu_monitor_analyze_data();
		pthread_cond_signal(&cond_ids[1]);
	}

	return NULL;
}

static void *thread_printer_func(void *data)
{
	UNUSED(data);
	/* TODO: function to block some signals */
	for (;;) {
		pthread_mutex_lock(&condlock);
		pthread_cond_wait(&cond_ids[1], &condlock);
		pthread_mutex_unlock(&condlock);

		cpu_monitor_print_res();
	}

	return NULL;
}

static void sig_handler(int signum)
{
	if (signum == SIGINT) {
		printf("Bye!\n");
		cpu_monitor_exit();
		exit(EXIT_SIGNAL + signum);
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

	pthread_mutex_init(&lock, NULL);
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
	for(i = 0; i < THR_NUM - 1; ++i)
		pthread_cond_destroy(&cond_ids[i]);
	cpu_monitor_exit();
	pthread_mutex_destroy(&condlock);
	pthread_mutex_destroy(&lock);
	return ret;
}
