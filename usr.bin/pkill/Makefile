#	$NetBSD: Makefile,v 1.2 2005/07/16 16:21:05 christos Exp $

PROG=	pkill
WARNS=	3

LDADD+=	-lkvm
DPADD+=	${LIBKVM}

LINKS+=	${BINDIR}/pkill ${BINDIR}/pgrep
MLINKS+=pkill.1 pgrep.1

.include <bsd.prog.mk>
