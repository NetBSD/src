#	$NetBSD: Makefile,v 1.35.2.1 2017/12/03 11:36:58 jdolecek Exp $

SUBDIR=	apm ata bluetooth dec dm dmover dtv filemon hdaudio hdmicec hpc \
	i2c i2o ic ieee1394 ir isa \
	microcode ofw pci pckbport pcmcia pud putter raidframe sbus scsipi \
	sun tc usb vme wscons

.include <bsd.own.mk>

.if ${MKISCSI} != "no"
SUBDIR+= iscsi
.endif

INCSDIR= /usr/include/dev

# Only install includes which are used by userland
INCS=	biovar.h ccdvar.h cgdvar.h fssvar.h keylock.h kttcpio.h lockstat.h \
	md.h vndvar.h spkrio.h

.include <bsd.kinc.mk>
