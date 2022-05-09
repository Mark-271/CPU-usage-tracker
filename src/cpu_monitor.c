// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Author: Mark Sungurov <mark.sungurov@gmail.com>
 */

#include <cpu_monitor.h>
#include <common.h>
#include <pthread.h>
#include <tools.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>

#define CPU_SAMPLING_DELAY	200 /* msec */
#define CPU_SAMPLING_NUM	(1000 / CPU_SAMPLING_DELAY)

/* CPU core usage data */
struct core_stat {
	char name[5];
	unsigned long long user;
	unsigned long long nice;
	unsigned long long system;
	unsigned long long idle;
	unsigned long long iowait;
	unsigned long long irq;
	unsigned long long softirq;
	unsigned long long steal;
};

struct cpu_usage {
	char name[5];
	/* Absolute values */
	unsigned long long idletime;
	unsigned long long worktime;
};

struct cpu_stat {
	const char *path;
	/* The number of cpu cores both physical and virtual */
	size_t cpu_num;
	struct core_stat *cs;
	struct cpu_usage *prev;
	long double *perc;
	bool print_ready;
	bool analyze_ready;
	bool thr_cancel; /* Flag indicating that all threads should exit */
	pthread_mutex_t lock;
	pthread_mutex_t condlock;
	pthread_cond_t cond_calc;
	pthread_cond_t cond_print;
};

static struct cpu_stat st; /* singleton */

static struct cpu_usage get_cpu_usage(struct core_stat obj)
{
	struct cpu_usage c;

	pthread_mutex_lock(&st.lock);
	strncpy(c.name, obj.name, sizeof(obj.name));
	c.name[sizeof(c.name) - 1] = '\0';
	c.idletime = obj.idle + obj.iowait;
	c.worktime = obj.user + obj.nice + obj.system +
		    obj.irq + obj.softirq + obj.steal;
	pthread_mutex_unlock(&st.lock);

	return c;
}

static long double get_cpuusage_delta(struct cpu_usage p, struct cpu_usage c)
{
	unsigned long long workingtime;
	unsigned long long totaltime;
	long double res;

	pthread_mutex_lock(&st.lock);
	workingtime = c. worktime - p.worktime;
	totaltime = workingtime + (c.idletime - p.idletime);
	pthread_mutex_unlock(&st.lock);

	res = (long double)workingtime / totaltime * 100.0L;

	return res;
}

static void print_perc(char *name, long double perc)
{
	long double average = perc / 5;
	char *colour;

	if (average > 70)
		colour = RED;
	else if (average < 30)
		colour = GRN;
	else
		colour = YEL;
	if ((strcmp(name, "cpu")) == 0)
		printf(UWHT "TOTAL:\t" RST "%s %.1Lf%%" RST "\n",
		       colour, average);
	else
		printf("%s:\t%s %.1Lf%%" RST "\n", name, colour, average);
}

/**
 * Calculate and aggregate the CPU usage percentage.
 */
static void cpu_monitor_analyze_data(void)
{
	struct cpu_usage cur;
	static size_t counter;

	if (st.analyze_ready) {

		pthread_mutex_lock(&st.lock);
		st.analyze_ready = false;
		pthread_mutex_unlock(&st.lock);

		if (counter == CPU_SAMPLING_NUM) {
			pthread_mutex_lock(&st.lock);
			st.print_ready = true;
			pthread_mutex_unlock(&st.lock);
			counter = 0;
			return;
		}

		for (size_t i = 0; i < st.cpu_num; i++) {
			cur = get_cpu_usage(st.cs[i]);
			st.perc[i] += get_cpuusage_delta(st.prev[i], cur);
			pthread_mutex_lock(&st.lock);
			st.prev[i] = cur;
			pthread_mutex_unlock(&st.lock);
		}

		counter++;
	}
}

/**
 * Print out both the total and per core CPU load values.
 */
static void cpu_monitor_print_res(void)
{
	if (st.print_ready) {

		pthread_mutex_lock(&st.lock);
		st.print_ready = false;
		pthread_mutex_unlock(&st.lock);

		clear_screen();

		pthread_mutex_lock(&st.lock);
		for (size_t i = 0; i < st.cpu_num; i++) {
			print_perc(st.prev[i].name, st.perc[i]);
			st.perc[i] = 0;
		}
		pthread_mutex_unlock(&st.lock);
	}
}

static bool file_exist(const char *path)
{
	return access(path, F_OK) != -1;
}

/**
 * Collect and parse raw data from the file pointed to by @ref path.
 *
 * @return 0 on success or -1 on error
 */
static int cpu_monitor_read_data(void)
{
	FILE *f;
	char *line = NULL;
	size_t len = 0;
	int err = -1;

	if (!file_exist(st.path)) {
		printf("Error: Incorrect path to file\n");
		return err;
	}

	f = fopen(st.path, "r");
	if (!f) {
		perror("fopen error\n");
		return err;
	}

	for (size_t i = 0; i < st.cpu_num; i++) {
		if (getline(&line, &len, f) < 0)
			goto err_getline;

		pthread_mutex_lock(&st.lock);
		sscanf(line, "%5s %llu %llu %llu %llu %llu %llu %llu %llu",
			     st.cs[i].name, &st.cs[i].user, &st.cs[i].nice,
			     &st.cs[i].system, &st.cs[i].idle, &st.cs[i].iowait,
			     &st.cs[i].irq, &st.cs[i].softirq, &st.cs[i].steal);
		pthread_mutex_unlock(&st.lock);
	}

	free(line);
	fclose(f);

	pthread_mutex_lock(&st.lock);
	st.analyze_ready = true;
	pthread_mutex_unlock(&st.lock);

	return 0;

err_getline:
	perror("getline error\n");
	free(line);
	fclose(f);
	return err;
}

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

/**
 * Thread 'Printer' function.
 *
 * Print cpu usage percentes for every cpu core to console.
 * Should be passed to pthread_create()
 *
 * @param data Pointer to user data
 * @return NULL
 */
void *thread_printer_func(void *data)
{
	UNUSED(data);
	thread_sig_mask();

	for (;;) {
		pthread_mutex_lock(&st.condlock);
		pthread_cond_wait(&st.cond_print, &st.condlock);
		pthread_mutex_unlock(&st.condlock);

		if (st.thr_cancel)
			break;

		cpu_monitor_print_res();
	}

	return NULL;
}

/**
 * Thread 'Analyzer' function.
 *
 * Calculate cpu usage percentage. Should be passed to pthread_create()
 *
 * @param data Pointer to user data
 * @return NULL
 */
void *thread_analyzer_func(void *data)
{
	UNUSED(data);
	thread_sig_mask();

	for (;;) {
		pthread_mutex_lock(&st.condlock);
		pthread_cond_wait(&st.cond_calc, &st.condlock);
		pthread_mutex_unlock(&st.condlock);

		if (st.thr_cancel) {
			pthread_cond_signal(&st.cond_print);
			break;
		}

		cpu_monitor_analyze_data();
		pthread_cond_signal(&st.cond_print);
	}

	return NULL;
}

/**
 * Thread 'Reader' function.
 *
 * Used to get and parse data from /proc/stat every 200 ms.
 * Should be passed to pthread_create()
 *
 * @param data Pointer to user data
 * @return NULL
 */
void *thread_reader_func(void *data)
{
	UNUSED(data);
	thread_sig_mask();

	for (;;) {
		int err;
		if (st.thr_cancel) {
			pthread_cond_signal(&st.cond_calc);
			break;
		}

		err = cpu_monitor_read_data();
		if (err) {
			printf("Error: thread_reader error\n");
			break;
		}
		pthread_cond_signal(&st.cond_calc);

		msleep(CPU_SAMPLING_DELAY);
	}

	return NULL;
}

/**
 * Signal handler.
 *
 * Callback used to close the application gently, when SIGINT signal is caught
 *
 * @param signum The signal number
 */
void sig_handler(int signum)
{
	if (signum == SIGINT) {
		st.thr_cancel = true;
		printf("Bye!\n");
	}
}

int cpu_monitor_init(void)
{
	st.path = "/proc/stat";
	st.cpu_num = (size_t)get_nprocs_conf() + 1;

	pthread_mutex_init(&st.lock, NULL);
	pthread_mutex_init(&st.condlock, NULL);

	pthread_cond_init(&st.cond_calc, NULL);
	pthread_cond_init(&st.cond_print, NULL);

	st.cs = (struct core_stat *)malloc(st.cpu_num *
					   sizeof(struct core_stat));
	if (!st.cs)
		goto err_malloc;

	st.prev = (struct cpu_usage *)malloc(st.cpu_num *
					     sizeof(struct cpu_usage));
	if (!st.prev) {
		free(st.cs);
		goto err_malloc;
	}

	st.perc = (long double *)malloc(st.cpu_num * sizeof(long double));
	if (!st.perc) {
		free(st.cs);
		free(st.prev);
		goto err_malloc;
	}
	memset(st.perc, 0, st.cpu_num * sizeof(long double));

	return 0;

err_malloc:
	perror("malloc error\n");
	return -1;
}

void cpu_monitor_exit(void)
{
	st.path = NULL;
	st.cpu_num = 0;

	pthread_cond_destroy(&st.cond_calc);
	pthread_cond_destroy(&st.cond_print);

	pthread_mutex_destroy(&st.lock);
	pthread_mutex_destroy(&st.condlock);

	free(st.cs);
	free(st.prev);
	free(st.perc);
	UNUSED(st);
}
