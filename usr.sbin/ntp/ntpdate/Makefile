#	$NetBSD: Makefile,v 1.2 2000/03/29 13:54:45 simonb Exp $

PROG=	ntpdate
MAN=	ntpdate.8
SRCS=	ntpdate.c

SRCS+=	version.c
CLEANFILES+=version.c version.o .version

.include <bsd.prog.mk>

version.c: ${OBJS:S/version.o//} ${LIBNTP}/libntp.a
	sh ${.CURDIR}/../scripts/mkver ntpd
