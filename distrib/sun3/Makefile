#	$NetBSD: Makefile,v 1.2.8.1 1999/06/21 03:47:38 perry Exp $

SUBDIR= ramdisk miniroot
TARGETS= all release

release:
	-mkdir -p ${RELEASEDIR}/installation/netboot
	cp -p ${DESTDIR}/usr/mdec/netboot \
	  ${RELEASEDIR}/installation/netboot/
	-mkdir -p ${RELEASEDIR}/installation/tapeimage
	cp -p ${DESTDIR}/usr/mdec/tapeboot \
	  ${RELEASEDIR}/installation/tapeimage/
	cp -p MakeBootTape MakeInstTape \
	  ${RELEASEDIR}/installation/tapeimage/

.include <bsd.subdir.mk>
