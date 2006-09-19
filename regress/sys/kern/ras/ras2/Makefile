#	$NetBSD: Makefile,v 1.3 2006/09/19 20:17:28 skrll Exp $

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

.if ${HAVE_GCC} == 4
CFLAGS+=	-fno-reorder-blocks
.endif
