#	$NetBSD: Makefile,v 1.6 2002/09/18 05:41:42 lukem Exp $

NOMAN=		# defined

PROG=		strptime
SRCS=		main.c
WARNS?=		1

CLEANFILES+=	got

regress:
	./strptime < ${.CURDIR}/tests > got
	diff ${.CURDIR}/expected got

.include <bsd.prog.mk>
