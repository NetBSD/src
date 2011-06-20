#	$NetBSD: Makefile,v 1.5 2011/06/20 07:43:59 mrg Exp $

NOMAN=	#defined

PROG=	ras1
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

