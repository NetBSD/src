#	$NetBSD: Makefile,v 1.4 2002/04/26 02:54:59 lukem Exp $

SUBDIR=		ramdisk miniroot
TARGETS+=	release

release:
	-mkdir -p ${RELEASEDIR}/installation/netboot
	${RELINSTALL} ${DESTDIR}/usr/mdec/bootyy ${DESTDIR}/usr/mdec/netboot \
	    ${RELEASEDIR}/installation/netboot
	-mkdir -p ${RELEASEDIR}/installation/tapeimage
	${RELINSTALL} ${DESTDIR}/usr/mdec/tapeboot \
	    ${.CURDIR}/MakeBootTape ${.CURDIR}/MakeInstTape \
	    ${RELEASEDIR}/installation/tapeimage

.include <bsd.subdir.mk>
