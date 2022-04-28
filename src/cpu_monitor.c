#include <cpu_monitor.h>
#include <file.h>
#include <tools.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>

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
	const char *path;
	/* The number of cpu cores both physical and virtual */
	size_t cpu_num;
	struct core_stat *cs;
};

/* Storage for statistical data obtained from CPU */
static struct cpu_stat st; /* singleton */


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

		sscanf(line, "%s %ld %ld %ld %ld %ld %ld %ld %ld",
			     st.cs[i].name, &st.cs[i].user, &st.cs[i].nice,
			     &st.cs[i].system, &st.cs[i].idle, &st.cs[i].iowait,
			     &st.cs[i].irq, &st.cs[i].softirq, &st.cs[i].steal);
	}
	free(line);
	fclose(f);

	return 0;

err_getline:
	perror("getline error\n");
	free(line);
	fclose(f);
	return err;
}

int cpu_monitor_init(void)
{
	int ret = 0;

	st.path = "/proc/stat";
	st.cpu_num = (size_t)get_nprocs_conf() + 1;

	st.cs = (struct core_stat *)malloc(st.cpu_num * sizeof(struct core_stat));
	if (!st.cs) {
		perror("malloc error\n");
		ret = -1;
	}

	return ret;
}

void cpu_monitor_exit(void)
{
	st.path = NULL;
	st.cpu_num = 0;

	free(st.cs);
	UNUSED(st);
}
