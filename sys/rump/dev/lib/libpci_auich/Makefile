#	$NetBSD: Makefile,v 1.3 2015/11/11 21:52:46 pooka Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_pci_auich
COMMENT=AC97 Intel Audio driver

IOCONF=	PCI_AUICH.ioconf
COMPONENT_SIMPLE= pci_auich

SRCS+=	auich.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/../dev
CPPFLAGS+= -I${.CURDIR}/../libaudio
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
