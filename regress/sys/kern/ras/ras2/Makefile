#	$NetBSD: Makefile,v 1.5.2.2 2014/05/22 11:37:18 yamt Exp $

NOMAN=	#defined

.include <bsd.own.mk>	# ACTIVE_CC

PROG=	ras2

regress:
	@if ./${PROG} ; then		\
		echo "PASSED";		\
	else				\
		echo "FAILED";		\
	fi

CFLAGS+=	${${ACTIVE_CC} == "gcc":? -fno-reorder-blocks :}

.include <bsd.prog.mk>
