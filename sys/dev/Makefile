#	$NetBSD: Makefile,v 1.31 2011/07/09 14:52:03 jmcneill Exp $

SUBDIR=	apm ata bluetooth dec dm dmover dtv hpc i2o ic ieee1394 ir isa \
	microcode ofw pci pckbport pcmcia pud putter raidframe sbus scsipi \
	sun tc usb vme wscons

INCSDIR= /usr/include/dev

# Only install includes which are used by userland
INCS=	biovar.h ccdvar.h cgdvar.h fssvar.h keylock.h kttcpio.h lockstat.h \
	md.h vndvar.h

.include <bsd.kinc.mk>
