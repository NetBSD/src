/*	$NetBSD: pfilter.c,v 1.8.6.1 2024/10/08 11:16:17 martin Exp $	*/
#include "namespace.h"
#include "includes.h"
#include "ssh.h"
#include "packet.h"
#include "log.h"
#include "pfilter.h"
#include <blocklist.h>
#include <syslog.h>

#ifndef SMALL
static struct blocklist *blstate;
#endif

#include "includes.h"
__RCSID("$NetBSD: pfilter.c,v 1.8.6.1 2024/10/08 11:16:17 martin Exp $");

void
pfilter_init()
{
#ifndef SMALL
	blstate = blocklist_open2(vsyslog_ss);
#endif
}

extern struct ssh *the_active_state;

void
pfilter_notify(int a)
{
#ifndef SMALL
	int fd;
	if (the_active_state == NULL)
		return;
	if (blstate == NULL)
		pfilter_init();
	if (blstate == NULL)
		return;
	// XXX: 3?
 	fd = ssh_packet_connection_is_on_socket(the_active_state) ?
	    ssh_packet_get_connection_in(the_active_state) : 3;
	(void)blocklist_r(blstate, a, fd, "ssh");
	if (a == 0) {
		blocklist_close(blstate);
		blstate = NULL;
	}
#else
	__USE(a);
#endif
}
