#	$NetBSD: Makefile,v 1.1.1.1 1999/01/18 07:37:24 mrg Exp $

PROG=	mksunbootcd
MAN=	mksunbootcd.1

.include <bsd.prog.mk>

test:	mksunbootcd
	echo Test > ./testcd
	./mksunbootcd -vd ./testcd *.c *.o

CLEANFILES+=	testcd
