#	$NetBSD: Makefile,v 1.2 2000/03/29 13:54:45 simonb Exp $

PROG=	ntptime
MAN=	ntptime.8
SRCS=	ntptime.c

SRCS+=	version.c
NTPSRCDIR=util
CLEANFILES+=version.c version.o .version

.include <bsd.prog.mk>

version.c: ${OBJS:S/version.o//} ${LIBNTP}/libntp.a
	sh ${.CURDIR}/../scripts/mkver ntpd
