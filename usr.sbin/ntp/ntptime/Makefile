#	$NetBSD: Makefile,v 1.1 2000/03/29 13:47:05 simonb Exp $

PROG=	ntptime
SRCS=	ntptime.c

SRCS+=	version.c
NTPSRCDIR=util
CLEANFILES+=version.c version.o .version

.include <bsd.prog.mk>

version.c: ${OBJS:S/version.o//} ${LIBNTP}/libntp.a
	sh ${.CURDIR}/../scripts/mkver ntpd
