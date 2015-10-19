#	$NetBSD: Makefile,v 1.3 2015/10/19 16:16:33 pooka Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci ${RUMPTOP}/../dev/usb

LIB=	rumpdev_pci_usbhc
COMMENT=PCI USB host controller drivers

IOCONF=	PCI_USBHC.ioconf
SRCS=	usbhc_at_pci.c

SRCS+=	ohci_pci.c ohci.c
SRCS+=	uhci_pci.c uhci.c
SRCS+=	ehci_pci.c ehci.c
SRCS+=	usb_pci.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/../dev

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
