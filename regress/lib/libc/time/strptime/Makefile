#	$NetBSD: Makefile,v 1.2 1998/01/22 19:20:22 thorpej Exp $

PROG=	strptime
SRCS=	main.c
NOMAN=	

CLEANFILES+=	got

regress:
	./strptime < ${.CURDIR}/tests > got
	diff ${.CURDIR}/expected got

.include <bsd.prog.mk>
