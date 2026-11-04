#	$NetBSD: Makefile,v 1.5.2.1 2026/11/04 23:28:41 perseant Exp $

PROG=	pkill

LDADD+=	-lkvm
DPADD+=	${LIBKVM}

LINKS+=	${BINDIR}/pkill ${BINDIR}/pgrep
LINKS+=	${BINDIR}/pkill ${BINDIR}/prenice
MLINKS+=pkill.1 pgrep.1
MLINKS+=pkill.1 prenice.1

COPTS.pkill.c+=		${CC_WNO_STRINGOP_OVERFLOW}
COPTS.pkill.c+=		${CC_WNO_CALLOC_TRANSPOSED_ARGS}

.include <bsd.prog.mk>
