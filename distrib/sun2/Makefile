#	$NetBSD: Makefile,v 1.1 2001/05/18 00:16:38 fredette Exp $

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
