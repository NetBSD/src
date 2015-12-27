#	$NetBSD: Makefile,v 1.1.2.1 2015/12/27 12:10:10 skrll Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_pci_eap
COMMENT=Ensoniq AudioPCI driver

IOCONF=	PCI_EAP.ioconf
RUMP_COMPONENT=ioconf

SRCS+=	eap.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/../dev
CPPFLAGS+= -I${.CURDIR}/../libaudio
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
