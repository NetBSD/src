#include "pfilter.h"
#include <stdio.h>	/* for NULL */
#include <blacklist.h>

static struct blacklist *blstate;

void
pfilter_notify(int a, int fd)
{
	if (blstate == NULL)
		blstate = blacklist_open();
	if (blstate == NULL)
		return;
	(void)blacklist_r(blstate, a, fd, "smtpd");
	if (a == 0) {
		blacklist_close(blstate);
		blstate = NULL;
	}
}
