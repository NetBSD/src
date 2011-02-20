#	$NetBSD: Makefile,v 1.1 2011/02/20 03:58:33 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_ath_pci
IOCONF=	if_ath_pci.ioconf

SRCS=	if_ath_pci.c

WARNS=	3

.include "../ath_hal/Makefile.inc"
.include <bsd.kmodule.mk>
