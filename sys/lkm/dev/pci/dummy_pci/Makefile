# $NetBSD: Makefile,v 1.2 2005/12/11 12:24:47 christos Exp $

S=	${.CURDIR}/../../../..

KMOD= dummy_pci
SRCS= dummy_pci_drv.c dummy_pci_lkm.c
NOMAN=	#yes

#CPPFLAGS+=	-DDIAGNOSTIC -DLOCKDEBUG
CPPFLAGS+=	-DDIAGNOSTIC -DDEBUG
WARNS=	2

.include <bsd.kmod.mk>
