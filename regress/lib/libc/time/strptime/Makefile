#	$NetBSD

PROG=	strptime
SRCS=	main.c
NOMAN=	

CLEANFILES+=	got

regress:
	./strptime < ${.CURDIR}/tests > got
	diff ${.CURDIR}/expected got

.include <bsd.prog.mk>
