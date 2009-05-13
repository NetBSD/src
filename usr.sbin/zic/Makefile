#	$NetBSD: Makefile,v 1.8.40.1 2009/05/13 19:20:46 jym Exp $

WARNS?=	2	# XXX -Wcast-qual in lib/libc/time

.include <bsd.own.mk>

PROG=	zic
SRCS=	zic.c scheck.c ialloc.c
MAN=	zic.8
CPPFLAGS+=-Dunix

.PATH: ${NETBSDSRCDIR}/lib/libc/time

.include <bsd.prog.mk>
