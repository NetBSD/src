#	$NetBSD: Makefile,v 1.3 2015/11/11 21:52:46 pooka Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_pci_if_iwn
COMMENT=Intel wireless device driver

IOCONF=	PCI_IF_IWN.ioconf
COMPONENT_SIMPLE= pci_if_iwn

SRCS+=	if_iwn.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
