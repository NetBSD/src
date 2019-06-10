# $NetBSD: Makefile,v 1.1.42.1 2019/06/10 22:09:34 christos Exp $

.include "../Makefile.inc"

.PATH:		${S}/net/npf

KMOD=		npf_ext_normalize

SRCS=		npf_ext_normalize.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.include <bsd.kmodule.mk>
