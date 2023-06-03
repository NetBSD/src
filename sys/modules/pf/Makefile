# $NetBSD: Makefile,v 1.18 2023/06/03 21:30:23 lukem Exp $

.include "../Makefile.inc"

.PATH:	${S}/dist/pf/net ${S}/dist/pf/netinet

KMOD=	pf
IOCONF=	pf.ioconf
SRCS=	if_compat.c
SRCS+=	if_pflog.c
SRCS+=	pf.c
SRCS+=	pf_if.c
SRCS+=	pf_ioctl.c
SRCS+=	pf_mtag.c
SRCS+=	pf_norm.c
SRCS+=	pf_osfp.c
SRCS+=	pf_ruleset.c
SRCS+=	pf_table.c
SRCS+=	tcp_rndiss.c

CPPFLAGS+=	-I${S}/dist/pf -I${S} -DINET6 -DINET

WARNS=	3

COPTS.pf.c+=		${CC_WNO_IMPLICIT_FALLTHROUGH} ${CC_WNO_ADDRESS_OF_PACKED_MEMBER}
COPTS.pf_norm.c+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}
COPTS.pf_table.c+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

.include <bsd.kmodule.mk>
