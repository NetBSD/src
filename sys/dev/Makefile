#	$NetBSD: Makefile,v 1.6.2.3 2002/01/08 00:29:17 nathanw Exp $

SUBDIR=	ata dec hpc i2o ic ir isa ofw pci pckbc pcmcia raidframe \
	sbus scsipi sun tc usb vme wscons 

KDIR=	/sys/dev
INCSDIR= /usr/include/dev

# Only install includes which are used by userland
INCS=	ccdvar.h md.h vndvar.h

.include <bsd.kinc.mk>
