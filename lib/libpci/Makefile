#	$NetBSD: Makefile,v 1.1 2001/09/13 22:52:19 thorpej Exp $

LIB=	pci

SRCS=	pci_bus.c pci_device.c pci_subr.c

CPPFLAGS+=-DPCIVERBOSE

.PATH.c: ${.CURDIR}/../../sys/dev/pci

MAN=	pci.3

MLINKS=	pci.3 pcibus_conf_read.3 \
	pci.3 pcibus_conf_write.3 \
	pci.3 pcidev_conf_read.3 \
	pci.3 pcidev_conf_write.3 \
	pci.3 pci_findvendor.3 \
	pci.3 pci_devinfo.3 \
	pci.3 pci_conf_print.3

INCSDIR= /usr/include
INCS=	pci.h

.include <bsd.lib.mk>
