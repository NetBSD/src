#	$NetBSD: Makefile,v 1.29 2009/08/15 14:22:54 uwe Exp $

SUBDIR=	apm ata bluetooth dec dmover hpc i2o ic ieee1394 ir isa microcode \
	ofw pci pckbport pcmcia pud putter raidframe sbus scsipi sun \
	tc usb vme wscons

INCSDIR= /usr/include/dev

# Only install includes which are used by userland
INCS=	biovar.h ccdvar.h cgdvar.h fssvar.h keylock.h kttcpio.h lockstat.h \
	md.h vndvar.h

.include <bsd.kinc.mk>
