#	$NetBSD: Makefile,v 1.10 2025/08/24 17:57:31 christos Exp $

.include <bsd.own.mk>

.if (defined(HOSTPROG) && \
     (${TOOLCHAIN_MISSING} == "no" || defined(EXTERNAL_TOOLCHAIN))) || \
    ${MKBINUTILS} != "no"

PROG=		dbsym
MAN=		dbsym.8

.include "${.PARSEDIR}/../bfd.mk"

.endif # MKBINUTILS != no
