#	$NetBSD: Makefile,v 1.4.16.2 2017/12/03 11:39:08 jdolecek Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_pci_auich
COMMENT=AC97 Intel Audio driver

IOCONF=	PCI_AUICH.ioconf
RUMP_COMPONENT=ioconf

SRCS+=	auich.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/../dev
CPPFLAGS+= -I${.CURDIR}/../libaudio
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
