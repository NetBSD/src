#	$NetBSD: Makefile,v 1.4 2001/10/04 16:09:11 chs Exp $

SUBDIR= ramdisk miniroot
TARGETS= all release

release:
	-mkdir -p ${RELEASEDIR}/installation/netboot
	cp -p ${DESTDIR}/usr/mdec/netboot \
	  ${RELEASEDIR}/installation/netboot/
	-mkdir -p ${RELEASEDIR}/installation/tapeimage
	cp -p ${DESTDIR}/usr/mdec/tapeboot \
	  ${RELEASEDIR}/installation/tapeimage/
	cp -p ${.CURDIR}/MakeBootTape ${.CURDIR}/MakeInstTape \
	  ${RELEASEDIR}/installation/tapeimage/

.include <bsd.subdir.mk>
