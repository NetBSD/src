#	$NetBSD: Makefile,v 1.1 2013/04/11 16:59:17 christos Exp $

LIB=event_pthreads

COPTS+=-pthread

SRCS= \
evthread_pthread.c

LIBDPLIBS+=     pthread  ${NETBSDSRCDIR}/lib/libpthread

.include <bsd.lib.mk>
