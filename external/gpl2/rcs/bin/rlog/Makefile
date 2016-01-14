#	$NetBSD: Makefile,v 1.1 2016/01/14 04:22:39 christos Exp $

PROG=	rlog
LINKS=	${BINDIR}/rlog ${BINDIR}/rcslog
MLINKS=	rlog.1 rcslog.1

CWARNFLAGS.clang+=	-Wno-string-plus-int

.include <bsd.prog.mk>
