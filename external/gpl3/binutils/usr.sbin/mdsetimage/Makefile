#	$NetBSD: Makefile,v 1.16 2024/07/04 02:20:03 christos Exp $

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
