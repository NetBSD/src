#	$NetBSD: Makefile,v 1.4 2015/11/16 23:27:07 pooka Exp $
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
