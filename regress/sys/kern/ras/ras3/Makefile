#	$NetBSD: Makefile,v 1.3 2006/09/02 21:27:41 martin Exp $

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

.if ${HAVE_GCC} == 4
CFLAGS+=	-fno-reorder-blocks
.endif
