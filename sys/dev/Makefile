#	$NetBSD: Makefile,v 1.8 2001/04/11 19:07:39 jdolecek Exp $

SUBDIR=	ata dec hpc i2o ic isa ofw pci pckbc pcmcia sbus scsipi sun tc	\
	usb vme wscons 

KDIR=	/sys/dev
INCSDIR= /usr/include/dev

# Only install includes which are used by userland
INCS=	ccdvar.h md.h vndvar.h

.include <bsd.kinc.mk>
