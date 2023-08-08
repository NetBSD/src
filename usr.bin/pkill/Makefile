#	$NetBSD: Makefile,v 1.5 2023/08/08 06:27:32 mrg Exp $

PROG=	pkill

LDADD+=	-lkvm
DPADD+=	${LIBKVM}

LINKS+=	${BINDIR}/pkill ${BINDIR}/pgrep
LINKS+=	${BINDIR}/pkill ${BINDIR}/prenice
MLINKS+=pkill.1 pgrep.1
MLINKS+=pkill.1 prenice.1

COPTS.pkill.c+=		${CC_WNO_STRINGOP_OVERFLOW}

.include <bsd.prog.mk>
