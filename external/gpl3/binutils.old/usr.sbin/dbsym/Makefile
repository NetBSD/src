#	$NetBSD: Makefile,v 1.1.1.3 2025/08/24 17:07:39 christos Exp $

.include <bsd.own.mk>

.if (defined(HOSTPROG) && \
     (${TOOLCHAIN_MISSING} == "no" || defined(EXTERNAL_TOOLCHAIN))) || \
    ${MKBINUTILS} != "no"

PROG=		dbsym
MAN=		dbsym.8

.include "${.PARSEDIR}/../bfd.mk"

.endif # MKBINUTILS != no
