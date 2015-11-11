#	$NetBSD: Makefile,v 1.3 2015/11/11 21:52:46 pooka Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_pci_if_wm
COMMENT=Intel GigE device driver

IOCONF=	PCI_IF_WM.ioconf
COMPONENT_SIMPLE= pci_if_wm

SRCS+=	if_wm.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
