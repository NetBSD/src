#include "namespace.h"
#include "ssh.h"
#include "packet.h"
#include "log.h"
#include "pfilter.h"
#include <blacklist.h>

static struct blacklist *blstate;

void
pfilter_init()
{
	blstate = blacklist_open();
}

void
pfilter_notify(int a)
{
	int fd;
	if (blstate == NULL)
		pfilter_init();
	if (blstate == NULL)
		return;
	// XXX: 3?
 	fd = packet_connection_is_on_socket() ? packet_get_connection_in() : 3;
	(void)blacklist_r(blstate, a, fd, "ssh");
}
