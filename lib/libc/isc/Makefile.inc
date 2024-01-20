#	$NetBSD: Makefile.inc,v 1.2 2024/01/20 14:52:47 christos Exp $

# net sources
.PATH: ${.CURDIR}/isc

SRCS+=	assertions.c ev_timers.c ev_streams.c

LINTFLAGS.ev_timers.c += -X 193 # XXX: lint is wrong
