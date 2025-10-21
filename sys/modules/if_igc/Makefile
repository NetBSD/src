# $NetBSD: Makefile,v 1.1 2025/10/21 07:23:05 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci/igc

KMOD=	if_igc
IOCONF=	if_igc.ioconf
SRCS=	if_igc.c
SRCS+=	igc_api.c
SRCS+=	igc_base.c
SRCS+=	igc_i225.c
SRCS+=	igc_mac.c
SRCS+=	igc_nvm.c
SRCS+=	igc_phy.c

.include <bsd.kmodule.mk>
