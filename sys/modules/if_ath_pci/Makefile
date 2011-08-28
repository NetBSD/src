#	$NetBSD: Makefile,v 1.2 2011/08/28 15:48:20 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_ath_pci
IOCONF=	if_ath_pci.ioconf

SRCS=	if_ath_pci.c

.include "../ath_hal/Makefile.inc"
.include <bsd.kmodule.mk>
