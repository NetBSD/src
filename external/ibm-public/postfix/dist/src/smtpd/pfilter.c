#include "pfilter.h"
#include <stdio.h>	/* for NULL */
#include <blocklist.h>

static struct blocklist *blstate;

void
pfilter_notify(int a, int fd)
{
	if (blstate == NULL)
		blstate = blocklist_open();
	if (blstate == NULL)
		return;
	(void)blocklist_r(blstate, a, fd, "smtpd");
	if (a == 0) {
		blocklist_close(blstate);
		blstate = NULL;
	}
}
