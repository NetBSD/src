#include "namespace.h"
#include "includes.h"
#include "ssh.h"
#include "packet.h"
#include "log.h"
#include "pfilter.h"
#include <blacklist.h>

#ifndef SMALL
static struct blacklist *blstate;
#endif

void
pfilter_init()
{
#ifndef SMALL
	blstate = blacklist_open();
#endif
}

void
pfilter_notify(int a)
{
#ifndef SMALL
	int fd;
	if (blstate == NULL)
		pfilter_init();
	if (blstate == NULL)
		return;
	// XXX: 3?
 	fd = packet_connection_is_on_socket() ? packet_get_connection_in() : 3;
	(void)blacklist_r(blstate, a, fd, "ssh");
	if (a == 0) {
		blacklist_close(blstate);
		blstate = NULL;
	}
#else
	__USE(a);
#endif
}
