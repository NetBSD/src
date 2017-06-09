#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#ifdef __linux__
#include <linux/limits.h>
#endif

#include "cron.h"

int close_all(int start)
{
#ifdef F_CLOSEM
	return fcntl(start, F_CLOSEM);
#else
	int fd, max;

	max = sysconf(_SC_OPEN_MAX);
	if (max <= 0)
		return -1;

#ifdef __linux__
	if (max < NR_OPEN)
		max = NR_OPEN;
#endif

	for (fd = start; fd < max; fd++) {
		if (close(fd) && errno != EBADF)
			return -1;
	}

	return 0;
#endif
}
