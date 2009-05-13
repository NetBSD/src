#	$NetBSD: Makefile,v 1.2.30.1 2009/05/13 19:20:01 jym Exp $

PROG=	pkill

LDADD+=	-lkvm
DPADD+=	${LIBKVM}

LINKS+=	${BINDIR}/pkill ${BINDIR}/pgrep
MLINKS+=pkill.1 pgrep.1

.include <bsd.prog.mk>
