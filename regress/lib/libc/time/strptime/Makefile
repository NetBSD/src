#	$NetBSD: Makefile,v 1.5 2001/12/12 01:24:04 tv Exp $

PROG=		strptime
SRCS=		main.c
NOMAN=		# defined
WARNS?=		1

CLEANFILES+=	got

regress:
	./strptime < ${.CURDIR}/tests > got
	diff ${.CURDIR}/expected got

.include <bsd.prog.mk>
