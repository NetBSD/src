#	$NetBSD: Makefile,v 1.6 2002/05/02 18:02:58 lukem Exp $

SUBDIR=		ramdisk miniroot
TARGETS+=	release

release: check_RELEASEDIR .WAIT \
	    ${DESTDIR}/usr/mdec/netboot ${DESTDIR}/usr/mdec/tapeboot \
	    MakeBootTape MakeInstTape
	${RELEASE_INSTALL} ${DESTDIR}/usr/mdec/netboot \
	    ${RELEASEDIR}/installation/netboot
	${RELEASE_INSTALL} ${DESTDIR}/usr/mdec/tapeboot \
	    ${RELEASEDIR}/installation/tapeimage
	${RELEASE_INSTALL} ${.CURDIR}/MakeBootTape ${.CURDIR}/MakeInstTape \
	    ${RELEASEDIR}/installation/tapeimage

.include <bsd.subdir.mk>
