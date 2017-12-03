#	$NetBSD: Makefile,v 1.1.10.3 2017/12/03 11:39:08 jdolecek Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_pci_if_wm
COMMENT=Intel GigE device driver

IOCONF=	PCI_IF_WM.ioconf
RUMP_COMPONENT=ioconf

SRCS+=	if_wm.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
