#	$NetBSD: Makefile,v 1.2 2015/10/19 16:16:33 pooka Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_pci_if_iwn
COMMENT=Intel wireless device driver

IOCONF=	PCI_IF_IWN.ioconf

SRCS=	iwn_at_pci.c

SRCS+=	if_iwn.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
