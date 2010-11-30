/*	$NetBSD: h_simpleserver.c,v 1.1 2010/11/30 22:09:15 pooka Exp $	*/

#include <sys/types.h>

#include <rump/rump.h>

#include <err.h>
#include <stdlib.h>
#include <unistd.h>

#define NOFAIL(e) do { int rv = e; if (rv) err(1, #e); } while (/*CONSTCOND*/0)

int
main(int argc, char *argv[])
{

	if (argc != 2)
		exit(1);

	NOFAIL(rump_daemonize_begin());
	NOFAIL(rump_init());
	NOFAIL(rump_init_server(argv[1]));
	NOFAIL(rump_daemonize_done(RUMP_DAEMONIZE_SUCCESS));

	for (;;)
		pause();
}
