# $NetBSD: Makefile,v 1.1.2.4 2004/09/21 13:36:24 skrll Exp $

S=	${.CURDIR}/../../../..

KMOD= dummy_pci
SRCS= dummy_pci_drv.c dummy_pci_lkm.c
NOMAN=	#yes

#CPPFLAGS+=	-DDIAGNOSTIC -DLOCKDEBUG
CPPFLAGS+=	-DDIAGNOSTIC -DDEBUG
WARNS=	2

.include <bsd.kmod.mk>
