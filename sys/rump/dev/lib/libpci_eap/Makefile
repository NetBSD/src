#	$NetBSD: Makefile,v 1.3 2015/11/11 21:52:46 pooka Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_pci_eap
COMMENT=Ensoniq AudioPCI driver

IOCONF=	PCI_EAP.ioconf
COMPONENT_SIMPLE= pci_eap

SRCS+=	eap.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/../dev
CPPFLAGS+= -I${.CURDIR}/../libaudio
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
