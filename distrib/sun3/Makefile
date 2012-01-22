#	$NetBSD: Makefile,v 1.11 2012/01/22 03:53:33 tsutsui Exp $

SUBDIR=		ramdisk miniroot .WAIT cdroms liveimage
TARGETS+=	release

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

release: check_RELEASEDIR .WAIT \
	    ${DESTDIR}/usr/mdec/netboot ${DESTDIR}/usr/mdec/tapeboot \
	    MakeBootTape MakeInstTape
	${RELEASE_INSTALL} \
	    ${DESTDIR}/usr/mdec/netboot \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/netboot
	${RELEASE_INSTALL} \
	    ${DESTDIR}/usr/mdec/tapeboot \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/tapeimage
	${RELEASE_INSTALL} \
	    ${.CURDIR}/MakeBootTape ${.CURDIR}/MakeInstTape \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/tapeimage

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

live_image:
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
