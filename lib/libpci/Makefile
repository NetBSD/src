#	$NetBSD: Makefile,v 1.4 2014/07/25 01:38:26 mrg Exp $

.include <bsd.own.mk>

LIB=	pci

SRCS=	pci_bus.c pci_device.c pci_drvname.c pci_subr.c pci_verbose.c

.PATH.c: ${NETBSDSRCDIR}/sys/dev/pci

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
