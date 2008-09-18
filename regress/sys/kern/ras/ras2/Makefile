#	$NetBSD: Makefile,v 1.3.18.1 2008/09/18 04:38:42 wrstuden Exp $

NOMAN=	#defined

PROG=	ras2
WARNS=	2

regress:
	@if ./${PROG} ; then		\
		echo "PASSED";		\
	else				\
		echo "FAILED";		\
	fi

.include <bsd.prog.mk>

.if defined(HAVE_GCC) && ${HAVE_GCC} == 4
CFLAGS+=	-fno-reorder-blocks
.endif
