#	$NetBSD: Makefile,v 1.2.4.2 2014/05/22 15:48:11 yamt Exp $

LIB=event_pthreads

COPTS+=-pthread

SRCS=		evthread_pthread.c

LIBDPLIBS+= \
	event	${.CURDIR}/../libevent \
	pthread	${NETBSDSRCDIR}/lib/libpthread

.include <bsd.lib.mk>
