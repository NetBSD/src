#	$NetBSD: Makefile,v 1.8.2.1 2002/01/10 19:52:43 thorpej Exp $

SUBDIR=	ata dec hpc i2o ic ir isa ofw pci pckbc pcmcia raidframe \
	sbus scsipi sun tc usb vme wscons 

KDIR=	/sys/dev
INCSDIR= /usr/include/dev

# Only install includes which are used by userland
INCS=	ccdvar.h md.h vndvar.h

.include <bsd.kinc.mk>
