#	$NetBSD: Makefile,v 1.3 2002/04/22 21:17:48 fredette Exp $

SUBDIR= ramdisk miniroot
TARGETS= all release

release:
	-mkdir -p ${RELEASEDIR}/installation/netboot
	cp -p ${DESTDIR}/usr/mdec/bootyy \
	  ${RELEASEDIR}/installation/netboot/
	cp -p ${DESTDIR}/usr/mdec/netboot \
	  ${RELEASEDIR}/installation/netboot/
	-mkdir -p ${RELEASEDIR}/installation/tapeimage
	cp -p ${DESTDIR}/usr/mdec/tapeboot \
	  ${RELEASEDIR}/installation/tapeimage/
	cp -p ${.CURDIR}/MakeBootTape ${.CURDIR}/MakeInstTape \
	  ${RELEASEDIR}/installation/tapeimage/

.include <bsd.subdir.mk>
