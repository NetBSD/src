#	$NetBSD: Makefile,v 1.9 2001/10/04 16:04:53 oster Exp $

SUBDIR=	ata dec hpc i2o ic isa ofw pci pckbc pcmcia raidframe \
	sbus scsipi sun tc usb vme wscons 

KDIR=	/sys/dev
INCSDIR= /usr/include/dev

# Only install includes which are used by userland
INCS=	ccdvar.h md.h vndvar.h

.include <bsd.kinc.mk>
