#	$NetBSD: Makefile,v 1.12 2002/10/04 18:33:03 elric Exp $

SUBDIR=	ata dec hpc i2o ic ir isa ofw pci pckbc pcmcia raidframe \
	sbus scsipi sun tc usb vme wscons 

KDIR=	/sys/dev
INCSDIR= /usr/include/dev

# Only install includes which are used by userland
INCS=	ccdvar.h cgdvar.h kttcpio.h md.h vndvar.h

.include <bsd.kinc.mk>
