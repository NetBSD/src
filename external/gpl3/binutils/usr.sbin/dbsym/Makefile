#	$NetBSD: Makefile,v 1.12.28.2 2025/08/02 05:25:20 perseant Exp $

.include <bsd.own.mk>

.if (defined(HOSTPROG) && \
     (${TOOLCHAIN_MISSING} == "no" || defined(EXTERNAL_TOOLCHAIN))) || \
    ${MKBINUTILS} != "no"

PROG=		dbsym
MAN=		dbsym.8

.include "${.PARSEDIR}/../bfd.mk"

.endif # MKBINUTILS != no
