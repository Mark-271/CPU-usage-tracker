#include <cpu_monitor.h>
#include <file.h>
#include <tools.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
	const char *path = "/proc/stat";
	cpu_monitor_init();

	while(1) {
		clear_screen();
		cpu_monitor_read_data(path);
		cpu_monitor_analyze_data();
		sleep(1);
	}

	cpu_monitor_exit();
	return EXIT_SUCCESS;
}
