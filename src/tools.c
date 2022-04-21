#include <tools.h>
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
	errno = 0;
	struct timespec wait = {
		.tv_sec = msec / 1000,
		.tv_nsec = (msec % 1000) * 1e6,
	};

	EINTR_LOOP(r, nanosleep(&wait, NULL));
}
