#	$NetBSD: Makefile,v 1.2 2002/03/06 12:29:57 lukem Exp $
#
#	Create RX50 floppies.

.include "../Makefile.inc"

all:
	cp ../inst/bootroot-${DISTRIBREV}.fs rx50-bootdisk-${DISTRIBREV}
	dd if=../inst/miniroot-${DISTRIBREV}.fs of=rx50-copy1-${DISTRIBREV} bs=512 count=800
	dd if=../inst/miniroot-${DISTRIBREV}.fs of=rx50-copy2-${DISTRIBREV} \
		bs=512 skip=800 count=800
	dd if=../inst/miniroot-${DISTRIBREV}.fs of=rx50-copy3-${DISTRIBREV} \
		bs=512 skip=1600 count=800
	dd if=../inst/miniroot-${DISTRIBREV}.fs of=rx50-copy4-${DISTRIBREV} \
		bs=512 skip=2400 count=800
	dd if=../inst/miniroot-${DISTRIBREV}.fs of=rx50-copy5-${DISTRIBREV} \
		bs=512 skip=3200 count=800
	dd if=../inst/miniroot-${DISTRIBREV}.fs of=rx50-copy6-${DISTRIBREV} \
		bs=512 skip=4000 count=800

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
