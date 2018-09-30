# $NetBSD: Makefile,v 1.1.42.1 2018/09/30 01:45:56 pgoyette Exp $

.include "../Makefile.inc"

.PATH:		${S}/net/npf

KMOD=		npf_ext_rndblock

SRCS=		npf_ext_rndblock.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.include <bsd.kmodule.mk>
