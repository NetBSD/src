#	$NetBSD: Makefile,v 1.2 1999/05/13 12:55:37 ross Exp $

PROG=	mksunbootcd
MAN=	mksunbootcd.1
WARNS=	1

.include <bsd.prog.mk>

test:	mksunbootcd
	echo Test > ./testcd
	./mksunbootcd -vd ./testcd *.c *.o

CLEANFILES+=	testcd
