#	$NetBSD: Makefile,v 1.2 2002/03/06 12:29:57 lukem Exp $
#
#	Create RX33 floppies.

.include "../Makefile.inc"

all:
	cp ../inst/bootroot-${DISTRIBREV}.fs rx33-bootdisk-${DISTRIBREV}
	dd if=../inst/miniroot-${DISTRIBREV}.fs of=rx33-copy1-${DISTRIBREV} bs=512 count=2400
	dd if=../inst/miniroot-${DISTRIBREV}.fs of=rx33-copy2-${DISTRIBREV} \
		bs=512 skip=2400 count=2400

.include <bsd.obj.mk>  
.include <bsd.subdir.mk>
