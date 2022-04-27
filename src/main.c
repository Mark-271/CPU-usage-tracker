#include <cpu_monitor.h>
#include <file.h>
#include <tools.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#define EXIT_SIGNAL	128	/* terminated by signal */

static void sig_handler(int signum)
{
	if (signum == SIGINT) {
		printf("Bye\n");
		cpu_monitor_exit();
		exit(EXIT_SIGNAL + signum);
	}
}

int main(void)
{
	const char *path = "/proc/stat";
	cpu_monitor_init();

	while(1) {
		if (signal(SIGINT, sig_handler) == SIG_ERR)
			fprintf(stderr, "Warning: Can't catch SIGINT\n");
		clear_screen();
		cpu_monitor_read_data(path);
		cpu_monitor_analyze_data();
		sleep(1);
	}

	return EXIT_SUCCESS;
}
