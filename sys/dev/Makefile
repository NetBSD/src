#	$NetBSD: Makefile,v 1.6.2.2 2001/10/22 20:41:14 nathanw Exp $

SUBDIR=	ata dec hpc i2o ic isa ofw pci pckbc pcmcia raidframe \
	sbus scsipi sun tc usb vme wscons 

KDIR=	/sys/dev
INCSDIR= /usr/include/dev

# Only install includes which are used by userland
INCS=	ccdvar.h md.h vndvar.h

.include <bsd.kinc.mk>
