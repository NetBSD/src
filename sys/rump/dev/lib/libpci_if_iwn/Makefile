#	$NetBSD: Makefile,v 1.1.6.2 2014/08/10 06:56:50 tls Exp $
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
