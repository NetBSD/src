#	$NetBSD: Makefile,v 1.5.2.1 2012/04/17 00:05:38 yamt Exp $

NOMAN=	#defined
PROG=	ras3
WARNS=	2

regress:
	@if ./${PROG} ; then		\
		echo "PASSED";		\
	else				\
		echo "FAILED";		\
	fi

CFLAGS+=	${${ACTIVE_CC} == "gcc":? -fno-reorder-blocks :}

.include <bsd.prog.mk>
