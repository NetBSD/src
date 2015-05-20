#	$NetBSD: Makefile,v 1.1 2015/05/20 12:21:38 pooka Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci ${RUMPTOP}/../dev/usb

LIB=	rumpdev_pci_usbhc
IOCONF=	PCI_USBHC.ioconf
SRCS=	usbhc_at_pci.c

SRCS+=	ohci_pci.c ohci.c
SRCS+=	uhci_pci.c uhci.c
SRCS+=	ehci_pci.c ehci.c
SRCS+=	usb_pci.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/../dev
CPPFLAGS+= -I${.CURDIR}/../libusb/opt

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
