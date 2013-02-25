# $NetBSD: Makefile,v 1.2.4.2 2013/02/25 00:30:24 tls Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/share/examples

TESTS_SH=	t_asm

.if ${MKKYUA} != "no"
# These examples come from external/bsd/lutok/tests/share/examples/lutok.
# Ensure they get registered in the Atffile only when installed.
ATFFILE_EXTRA_TPS=	lutok
.endif

.include <bsd.test.mk>
