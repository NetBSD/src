#	$NetBSD: Makefile,v 1.1 2000/03/29 13:13:45 simonb Exp $

PROG=	ntptrace
SRCS=	ntptrace.c

SRCS+=	version.c
CLEANFILES+=version.c version.o .version

.include <bsd.prog.mk>

version.c: ${OBJS:S/version.o//} ${LIBNTP}/libntp.a
	sh ${.CURDIR}/../scripts/mkver ntpd
