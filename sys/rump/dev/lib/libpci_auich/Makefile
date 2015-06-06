#	$NetBSD: Makefile,v 1.1.2.2 2015/06/06 14:40:27 skrll Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_pci_auich
IOCONF=	PCI_AUICH.ioconf
SRCS=	auich_at_pci.c
SRCS+=	auich.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/../dev
CPPFLAGS+= -I${.CURDIR}/../libaudio
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
