#	$NetBSD: Makefile,v 1.2 2001/12/17 20:27:39 fredette Exp $

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
