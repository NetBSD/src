# $NetBSD: Makefile,v 1.1 2004/08/18 14:06:13 drochner Exp $

S=	${.CURDIR}/../../../..

KMOD= dummy_pci
SRCS= dummy_pci_drv.c dummy_pci_lkm.c
NOMAN=	#yes

#CPPFLAGS+=	-DDIAGNOSTIC -DLOCKDEBUG
CPPFLAGS+=	-DDIAGNOSTIC -DDEBUG
WARNS=	2

.include <bsd.kmod.mk>
