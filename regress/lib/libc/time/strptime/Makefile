#	$NetBSD: Makefile,v 1.4 1999/02/13 02:54:32 lukem Exp $

PROG=	strptime
SRCS=	main.c
MKMAN=	no
WARNS?=	1

CLEANFILES+=	got

regress:
	./strptime < ${.CURDIR}/tests > got
	diff ${.CURDIR}/expected got

.include <bsd.prog.mk>
