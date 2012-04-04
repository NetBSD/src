#	$NetBSD: Makefile,v 1.6 2012/04/04 10:59:47 joerg Exp $

NOMAN=	#defined

PROG=	ras2
WARNS=	2

regress:
	@if ./${PROG} ; then		\
		echo "PASSED";		\
	else				\
		echo "FAILED";		\
	fi

CFLAGS+=	${${ACTIVE_CC} == "gcc":? -fno-reorder-blocks :}

.include <bsd.prog.mk>
