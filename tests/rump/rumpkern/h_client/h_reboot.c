/*	$NetBSD: h_reboot.c,v 1.1 2010/11/30 22:09:15 pooka Exp $	*/

#include <sys/types.h>

#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <rump/rump_syscalls.h>
#include <rump/rumpclient.h>

int
main(void)
{

	if (rumpclient_init() == -1)
		err(1, "rumpclient init");

	/* returns ENOENT, since server dies */
	rump_sys_reboot(0, NULL);
}
