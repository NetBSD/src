#	$NetBSD: Makefile,v 1.14 2024/07/04 02:20:03 christos Exp $

.include <bsd.own.mk>

.if (defined(HOSTPROG) && \
     (${TOOLCHAIN_MISSING} == "no" || defined(EXTERNAL_TOOLCHAIN))) || \
    ${MKBINUTILS} != "no"

PROG=		dbsym
MAN=		dbsym.8

.include "${.PARSEDIR}/../bfd.mk"

.endif # MKBINUTILS != no
