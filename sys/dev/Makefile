#	$NetBSD: Makefile,v 1.6.2.1 2001/06/21 20:01:13 nathanw Exp $

SUBDIR=	ata dec hpc i2o ic isa ofw pci pckbc pcmcia sbus scsipi sun tc	\
	usb vme wscons 

KDIR=	/sys/dev
INCSDIR= /usr/include/dev

# Only install includes which are used by userland
INCS=	ccdvar.h md.h vndvar.h

.include <bsd.kinc.mk>
