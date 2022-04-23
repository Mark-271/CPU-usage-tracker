#include <cpu_monitor.h>
#include <file.h>
#include <tools.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>

/* Storage for statistical data obtained from CPU */
struct cpu_stat st;

/**
 * Collect and parse raw data from the file pointed to by @ref path.
 *
 * Caller must free allocated buffer.
 *
 * @param path Path to the file '/proc/stat'
 * @return 0 on success or -1 on error
 */
int cpu_monitor_read_data(const char *path)
{
	FILE *f;
	char *line = NULL;
	size_t n;
	size_t len = 0;
	struct core_stat *buf;
	int err = -1;

	if (!file_exist(path)) {
			printf("Error: Incorrect path to file\n");
			return err;
	}

	n = (size_t)get_nprocs_conf() + 1;
	buf = (struct core_stat *)malloc(n * sizeof(struct core_stat));
	if (!buf) {
		perror("malloc error\n");
		return err;
	}

	f = fopen(path, "r");
	if (!f) {
		perror("fopen error\n");
		return err;
	}

	for (size_t i = 0; i < n; i++) {
		if (getline(&line, &len, f) < 0)
			goto err_getline;

		sscanf(line, "%s %ld %ld %ld %ld %ld %ld %ld %ld",
			     buf[i].name, &buf[i].user, &buf[i].nice,
			     &buf[i].system, &buf[i].idle, &buf[i].iowait,
			     &buf[i].irq, &buf[i].softirq, &buf[i].steal);
	}
	free(line);

	st.cpu_num = n;
	st.cs = buf;

	return 0;

err_getline:
	free(line);
	perror("getline error\n");
	return err;
}
