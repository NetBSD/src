#	$NetBSD: Makefile,v 1.6 2025/09/18 03:36:01 mrg Exp $

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
