#	$NetBSD: Makefile,v 1.4 2001/12/12 00:05:10 tv Exp $

PROG=		mksunbootcd
WARNS=		1

.include <bsd.prog.mk>

test:	mksunbootcd
	echo Test > ./testcd
	./mksunbootcd -vd ./testcd *.c *.o

CLEANFILES+=	testcd
