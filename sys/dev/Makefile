#	$NetBSD: Makefile,v 1.3.14.4 2001/04/21 17:48:15 bouyer Exp $

SUBDIR=	ata dec hpc i2o ic isa ofw pci pckbc pcmcia sbus scsipi sun tc	\
	usb vme wscons 

KDIR=	/sys/dev
INCSDIR= /usr/include/dev

# Only install includes which are used by userland
INCS=	ccdvar.h md.h vndvar.h

.include <bsd.kinc.mk>
