#	$NetBSD: Makefile,v 1.2 2000/03/29 13:54:46 simonb Exp $

PROG=	ntptrace
MAN=	ntptrace.8
SRCS=	ntptrace.c

SRCS+=	version.c
CLEANFILES+=version.c version.o .version

.include <bsd.prog.mk>

version.c: ${OBJS:S/version.o//} ${LIBNTP}/libntp.a
	sh ${.CURDIR}/../scripts/mkver ntpd
