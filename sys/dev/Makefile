#	$NetBSD: Makefile,v 1.47 2020/09/27 11:57:28 jmcneill Exp $

SUBDIR=	apm ata bluetooth dec dm dmover dtv hdaudio hdmicec hid hpc \
	i2c i2o ic ieee1394 ir isa \
	microcode ofw pci pckbport pcmcia pud putter raidframe sbus scsipi \
	spi sun tc usb vme wscons

.include <bsd.own.mk>

.if ${HAVE_NVMM:Uno} == "yes"
SUBDIR+= nvmm
.endif

.if ${MKISCSI} != "no"
SUBDIR+= iscsi
.endif

INCSDIR= /usr/include/dev

# Only install includes which are used by userland
INCS=	biovar.h ccdvar.h cgdvar.h fssvar.h keylock.h kttcpio.h lockstat.h \
	md.h vndvar.h spkrio.h

.include <bsd.kinc.mk>
