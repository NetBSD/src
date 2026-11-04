#	$NetBSD: Makefile,v 1.9.2.1 2026/11/04 22:46:36 perseant Exp $

.include <bsd.init.mk>

MDSETIMAGE=${NETBSDSRCDIR}/usr.sbin/mdsetimage
.PATH: ${MDSETIMAGE}

.if (defined(HOSTPROG) && \
     (${TOOLCHAIN_MISSING} == "no" || defined(EXTERNAL_TOOLCHAIN))) || \
    ${MKBINUTILS} != "no"

PROG=		mdsetimage
SRCS=		mdsetimage.c bin_bfd.c
MAN=		mdsetimage.8

CPPFLAGS+=	-I${MDSETIMAGE}

.include "${.PARSEDIR}/../bfd.mk"

.endif # MKBINUTILS != no
