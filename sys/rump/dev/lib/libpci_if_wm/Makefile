#	$NetBSD: Makefile,v 1.2 2015/10/19 16:16:33 pooka Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_pci_if_wm
COMMENT=Intel GigE device driver

IOCONF=	PCI_IF_WM.ioconf

SRCS=	wm_at_pci.c

SRCS+=	if_wm.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
