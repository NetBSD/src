#	$NetBSD: Makefile,v 1.4 2010/12/06 21:16:54 mrg Exp $

PROG=	pkill

LDADD+=	-lkvm
DPADD+=	${LIBKVM}

LINKS+=	${BINDIR}/pkill ${BINDIR}/pgrep
LINKS+=	${BINDIR}/pkill ${BINDIR}/prenice
MLINKS+=pkill.1 pgrep.1
MLINKS+=pkill.1 prenice.1

.include <bsd.prog.mk>
