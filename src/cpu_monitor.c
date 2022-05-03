#include <cpu_monitor.h>
#include <threads.h>
#include <file.h>
#include <common.h>
#include <tools.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <stdbool.h>

/* CPU core usage data */
struct  core_stat {
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
};

/* Storage for statistical data obtained from CPU */
static struct cpu_stat st; /* singleton */

static struct cpu_usage get_cpu_usage(struct core_stat obj)
{
	struct cpu_usage c;

	pthread_mutex_lock(&lock);
	strncpy(c.name, obj.name, sizeof(obj.name));
	c.name[sizeof(c.name) - 1] = '\0';
	c.idletime = obj.idle + obj.iowait;
	c.worktime = obj.user + obj.nice + obj.system +
		    obj.irq + obj.softirq + obj.steal;
	pthread_mutex_unlock(&lock);

	return c;
}

static long double get_cpuusage_delta(struct cpu_usage prev, struct cpu_usage cur)
{
	/*
	 * PrevIdle = previdle + previowait
	 * Idle = idle + iowait
	 * PrevNonIdle = prevuser + prevnice + prevsystem + previrq
	 * 		 + prevsoftirq + prevsteal
	 * NonIdle = user + nice + system + irq + softirq + steal
	 *
	 * PrevTotal = PrevIdle + PrevNonIdle 0
	 * Total = Idle + NonIdle 3157173
	 * totald = Total - PrevTotal 3157173
	 * idled = Idle - PrevIdle 2723326
	 * CPU_Percentage = (totald - idled)/totald
	 */
	unsigned long long workingtime;
	unsigned long long totaltime;
	long double res;

	pthread_mutex_lock(&lock);
	workingtime = cur. worktime - prev.worktime;
	totaltime = workingtime + (cur.idletime - prev.idletime);
	pthread_mutex_unlock(&lock);

	res = (long double)workingtime / totaltime * 100.0L;

	return res;
}

void cpu_monitor_analyze_data(void)
{
	struct cpu_usage cur[st.cpu_num];
	static size_t counter;

	if (st.analyze_ready) {

		pthread_mutex_lock(&lock);
		st.analyze_ready = false;
		pthread_mutex_unlock(&lock);

		if (counter == 5) {
			pthread_mutex_lock(&lock);
			st.print_ready = true;
			pthread_mutex_unlock(&lock);
			counter = 0;
			return;
		}

		for (size_t i = 0; i < st.cpu_num; i++) {
			cur[i] = get_cpu_usage(st.cs[i]);
			st.perc[i] += get_cpuusage_delta(st.prev[i], cur[i]);
			pthread_mutex_lock(&lock);
			st.prev[i] = cur[i];
			pthread_mutex_unlock(&lock);
		}

		counter++;
	}
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
		printf(UWHT "TOTAL:\t" RST "%s %.1Lf%%" RST "\n",colour, average);
	else
		printf("%s:\t%s %.1Lf%%" RST "\n", name, colour, average);
}

void cpu_monitor_print_res(void)
{
	if (st.print_ready) {

		pthread_mutex_lock(&lock);
		st.print_ready = false;
		pthread_mutex_unlock(&lock);

		clear_screen();

		pthread_mutex_lock(&condlock);
		for (size_t i = 0; i < st.cpu_num; i++) {
			print_perc(st.prev[i].name, st.perc[i]);
			st.perc[i] = 0;
		}
		pthread_mutex_unlock(&condlock);
	}
}

/**
 * Collect and parse raw data from the file pointed to by @ref path.
 *
 * Caller must free allocated buffer.
 *
 * @return 0 on success or -1 on error
 */
int cpu_monitor_read_data(void)
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

		pthread_mutex_lock(&lock);
		sscanf(line, "%s %lld %lld %lld %lld %lld %lld %lld %lld",
			     st.cs[i].name, &st.cs[i].user, &st.cs[i].nice,
			     &st.cs[i].system, &st.cs[i].idle, &st.cs[i].iowait,
			     &st.cs[i].irq, &st.cs[i].softirq, &st.cs[i].steal);
		pthread_mutex_unlock(&lock);
	}

	free(line);
	fclose(f);

	pthread_mutex_lock(&lock);
	st.analyze_ready = true;
	pthread_mutex_unlock(&lock);

	return 0;

err_getline:
	perror("getline error\n");
	free(line);
	fclose(f);
	return err;
}

int cpu_monitor_init(void)
{
	st.path = "/proc/stat";
	st.cpu_num = (size_t)get_nprocs_conf() + 1;

	st.cs = (struct core_stat *)malloc(st.cpu_num * sizeof(struct core_stat));
	if (!st.cs) {
		goto err_malloc;
	}

	st.prev = (struct cpu_usage *)malloc(st.cpu_num * sizeof(struct cpu_usage));
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

	free(st.cs);
	free(st.prev);
	free(st.perc);
	UNUSED(st);
}
