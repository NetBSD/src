#	$NetBSD: Makefile,v 1.27.14.2 2008/04/03 11:14:48 mjf Exp $

SUBDIR=	apm ata bluetooth dec devfsctl dmover hpc i2o ic ieee1394 ir isa \
	microcode \
	ofw pci pckbport pcmcia pud putter raidframe sbus scsipi sun tc usb \
	vme wscons

INCSDIR= /usr/include/dev

# Only install includes which are used by userland
INCS=	biovar.h ccdvar.h cgdvar.h fssvar.h kttcpio.h lockstat.h md.h vndvar.h

.include <bsd.kinc.mk>
