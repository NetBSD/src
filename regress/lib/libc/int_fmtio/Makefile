#	$NetBSD: Makefile,v 1.4 2011/05/26 12:56:30 joerg Exp $

NOMAN=		# defined

PROG=		fmtio_check
WARNS=		2 # need this!

regress: ${PROG}
	./${PROG}

CWARNFLAGS.clang+=	-Wno-uninitialized

.include <bsd.prog.mk>
