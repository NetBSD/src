#	$NetBSD: Makefile,v 1.5 2002/04/26 15:05:29 lukem Exp $

SUBDIR= ramdisk miniroot
TARGETS= all release

release:
	-mkdir -p ${RELEASEDIR}/installation/netboot
	${RELINSTALL} ${DESTDIR}/usr/mdec/netboot \
	    ${RELEASEDIR}/installation/netboot
	-mkdir -p ${RELEASEDIR}/installation/tapeimage
	${RELINSTALL} ${DESTDIR}/usr/mdec/tapeboot \
	    ${RELEASEDIR}/installation/tapeimage
	${RELINSTALL} ${.CURDIR}/MakeBootTape ${.CURDIR}/MakeInstTape \
	    ${RELEASEDIR}/installation/tapeimage

.include <bsd.subdir.mk>
