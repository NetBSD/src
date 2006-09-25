#	$NetBSD: Makefile,v 1.2.12.1 2006/09/25 00:10:02 riz Exp $

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
