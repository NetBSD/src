#include <time.h>

#include "cron.h"

/* Sat, 27 Feb 93 11:44:51 CST
 * 123456789012345678901234567
 */

char *
arpadate(
	 time_t *clock
) {
	time_t t = clock ?*clock :time();
	struct tm *tm = localtime(&t);
	static char ret[30];	/* zone name might be >3 chars */
	
	(void) sprintf(ret, "%s, %2d %s %2d %02d:%02d:%02d %s",
		       DowNames[tm->tm_wday],
		       tm->tm_mday,
		       MonthNames[tm->tm_mon],
		       tm->tm_year,
		       tm->tm_hour,
		       tm->tm_min,
		       tm->tm_sec,
		       tm->tm_zone);
	return ret;
}
