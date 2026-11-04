#	$NetBSD: Makefile,v 1.9.2.1 2026/11/04 22:46:36 perseant Exp $

.include <bsd.own.mk>

.if (defined(HOSTPROG) && \
     (${TOOLCHAIN_MISSING} == "no" || defined(EXTERNAL_TOOLCHAIN))) || \
    ${MKBINUTILS} != "no"

PROG=		dbsym
MAN=		dbsym.8

.include "${.PARSEDIR}/../bfd.mk"

.endif # MKBINUTILS != no
