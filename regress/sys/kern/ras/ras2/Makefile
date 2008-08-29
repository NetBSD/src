#	$NetBSD: Makefile,v 1.4 2008/08/29 00:02:23 gmcgarry Exp $

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
