#	$NetBSD: Makefile,v 1.3 2011/05/26 12:56:30 joerg Exp $

NOMAN=		# defined

PROG=		sleeptest

regress: ${PROG}
	./sleeptest

CWARNFLAGS.clang+=	-Wno-format

.include <bsd.prog.mk>
