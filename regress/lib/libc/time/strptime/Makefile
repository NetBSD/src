#	$NetBSD: Makefile,v 1.3 1998/01/22 19:25:15 thorpej Exp $

PROG=	strptime
SRCS=	main.c
NOMAN=	
WARNS?=	1

CLEANFILES+=	got

regress:
	./strptime < ${.CURDIR}/tests > got
	diff ${.CURDIR}/expected got

.include <bsd.prog.mk>
