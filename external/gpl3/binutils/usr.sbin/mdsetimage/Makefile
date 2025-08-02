#	$NetBSD: Makefile,v 1.13.26.2 2025/08/02 05:25:20 perseant Exp $

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
