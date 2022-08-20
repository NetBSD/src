#	$NetBSD: Makefile,v 1.48 2022/08/20 11:42:49 riastradh Exp $

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
INCS+=	biovar.h
INCS+=	ccdvar.h
INCS+=	cgdvar.h
INCS+=	fssvar.h
INCS+=	keylock.h
INCS+=	kttcpio.h
INCS+=	lockstat.h
INCS+=	md.h
INCS+=	spkrio.h
INCS+=	vndvar.h

.include <bsd.kinc.mk>
