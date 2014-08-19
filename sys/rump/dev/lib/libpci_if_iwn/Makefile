#	$NetBSD: Makefile,v 1.1.10.2 2014/08/20 00:04:38 tls Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_pci_if_iwn
IOCONF=	PCI_IF_IWN.ioconf

SRCS=	iwn_at_pci.c

SRCS+=	if_iwn.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
