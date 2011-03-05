#	$NetBSD: Makefile,v 1.1.2.2 2011/03/05 15:10:45 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_ath_pci
IOCONF=	if_ath_pci.ioconf

SRCS=	if_ath_pci.c

WARNS=	3

.include "../ath_hal/Makefile.inc"
.include <bsd.kmodule.mk>
