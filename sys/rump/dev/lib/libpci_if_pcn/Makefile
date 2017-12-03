#	$NetBSD: Makefile,v 1.4.16.2 2017/12/03 11:39:08 jdolecek Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_pci_if_pcn
COMMENT=PCnet Ethernet device driver

IOCONF=	PCI_IF_PCN.ioconf
RUMP_COMPONENT=ioconf

SRCS+=	if_pcn.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
