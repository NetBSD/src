#	$NetBSD: Makefile,v 1.1.4.2 2013/06/23 06:28:19 tls Exp $

LIB=event_pthreads

COPTS+=-pthread

SRCS= \
evthread_pthread.c

LIBDPLIBS+=     pthread  ${NETBSDSRCDIR}/lib/libpthread

.include <bsd.lib.mk>
