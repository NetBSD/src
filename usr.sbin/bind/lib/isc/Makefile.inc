# $NetBSD: Makefile.inc,v 1.1.1.1 1998/10/05 18:01:58 tron Exp $

.PATH: ${.CURDIR}/isc

SRCS+=	tree.c base64.c bitncmp.c assertions.c memcluster.c logging.c heap.c \
	eventlib.c ev_connects.c ev_files.c \
        ev_timers.c ev_streams.c ev_waits.c
