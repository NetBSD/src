/*	$NetBSD: h_simplecli.c,v 1.1 2010/11/30 22:09:15 pooka Exp $	*/

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

	if (rump_sys_getpid() > 0)
		exit(0);
	err(1, "getpid");
}
