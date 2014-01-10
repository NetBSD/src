#	$NetBSD: Makefile,v 1.7 2014/01/10 15:00:16 martin Exp $

.include <bsd.own.mk>	# ACVTIVE_CC

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
