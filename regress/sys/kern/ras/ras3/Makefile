#	$NetBSD: Makefile,v 1.4.18.1 2011/06/23 14:18:41 cherry Exp $

NOMAN=	#defined
PROG=	ras3
WARNS=	2

regress:
	@if ./${PROG} ; then		\
		echo "PASSED";		\
	else				\
		echo "FAILED";		\
	fi

.include <bsd.prog.mk>

.if defined(HAVE_GCC)
CFLAGS+=	-fno-reorder-blocks
.endif
