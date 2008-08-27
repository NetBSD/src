#include <err.h>
#include <errno.h>
#include <string.h>
#include <time.h>

int
main(void)
{
	time_t rc;
	struct tm tms;

	(void)memset(&tms, 0, sizeof(tms));
	tms.tm_year = ~0;

	errno = 0;
	rc = mktime(&tms);

	if (errno != 0)
		errx(1, "mktime(): errno was %d", errno);

	return 0;
}
