#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <err.h>


int main(void)
{
	DIR *dp;
	long loc;
	void *memused;
	int i;
	int oktouse = 4096;

	dp = opendir(".");
	if (dp == NULL)
		err(EXIT_FAILURE, "Could not open current directory");
	loc = telldir(dp);
	memused = sbrk(0);
	closedir(dp);

	for (i=0; i<1000; i++) {
		dp = opendir(".");
		if (dp == NULL)
			err(EXIT_FAILURE, "Could not open current directory");
		loc = telldir(dp);
		closedir(dp);

		if (sbrk(0) - memused > oktouse) {
			warnx("Used %d extra bytes for %d telldir calls",
				(sbrk(0)-memused), i);
			oktouse = sbrk(0) - memused;
		}
	}
	if (oktouse > 4096) {
		errx(EXIT_FAILURE, "Failure: leaked %d bytes", oktouse);
#ifdef DEBUG
	} else {
		printf("OK: used %d bytes\n", sbrk(0)-memused);
#endif
	}
	return 0;
}
