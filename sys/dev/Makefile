#	$NetBSD: Makefile,v 1.11 2002/06/28 23:27:14 thorpej Exp $

SUBDIR=	ata dec hpc i2o ic ir isa ofw pci pckbc pcmcia raidframe \
	sbus scsipi sun tc usb vme wscons 

KDIR=	/sys/dev
INCSDIR= /usr/include/dev

# Only install includes which are used by userland
INCS=	ccdvar.h kttcpio.h md.h vndvar.h

.include <bsd.kinc.mk>
