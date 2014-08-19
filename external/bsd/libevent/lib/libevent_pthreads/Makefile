#	$NetBSD: Makefile,v 1.1.4.3 2014/08/19 23:47:16 tls Exp $

LIB=event_pthreads

COPTS+=-pthread

SRCS=		evthread_pthread.c

LIBDPLIBS+= \
	event	${.CURDIR}/../libevent \
	pthread	${NETBSDSRCDIR}/lib/libpthread

.include <bsd.lib.mk>
