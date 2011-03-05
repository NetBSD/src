#	$NetBSD: Makefile,v 1.1.4.2 2011/03/05 20:55:45 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_ath_pci
IOCONF=	if_ath_pci.ioconf

SRCS=	if_ath_pci.c

WARNS=	3

.include "../ath_hal/Makefile.inc"
.include <bsd.kmodule.mk>
