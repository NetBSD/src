#	$NetBSD: Makefile,v 1.3 2000/05/18 01:10:24 matt Exp $

PROG=	mksunbootcd
MAN=	mksunbootcd.1
WARNS=	1

LDSTATIC?=-static

.include <bsd.prog.mk>

test:	mksunbootcd
	echo Test > ./testcd
	./mksunbootcd -vd ./testcd *.c *.o

CLEANFILES+=	testcd
