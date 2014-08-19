#	$NetBSD: Makefile,v 1.6.2.1 2014/08/20 00:02:23 tls Exp $

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
