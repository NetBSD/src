#	$NetBSD: Makefile,v 1.1 2000/03/29 13:13:45 simonb Exp $

PROG=	ntptimeset
SRCS=	ntptimeset.c ntptime_config.c

SRCS+=	version.c
NTPSRCDIR=ntpdate
CLEANFILES+=version.c version.o .version

.include <bsd.prog.mk>

version.c: ${OBJS:S/version.o//} ${LIBNTP}/libntp.a
	sh ${.CURDIR}/../scripts/mkver ntpd
