# $NetBSD: Makefile,v 1.2 2018/09/29 14:41:35 rmind Exp $

.include "../Makefile.inc"

.PATH:		${S}/net/npf

KMOD=		npf_ext_normalize

SRCS=		npf_ext_normalize.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.include <bsd.kmodule.mk>
