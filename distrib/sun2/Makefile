#	$NetBSD: Makefile,v 1.7.16.1 2007/03/09 06:59:27 riz Exp $

SUBDIR=		ramdisk miniroot cdroms
TARGETS+=	release

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

release: check_RELEASEDIR .WAIT \
	    ${DESTDIR}/usr/mdec/bootyy ${DESTDIR}/usr/mdec/netboot \
	    ${DESTDIR}/usr/mdec/tapeboot MakeBootTape MakeInstTape
	${RELEASE_INSTALL} \
	    ${DESTDIR}/usr/mdec/bootyy ${DESTDIR}/usr/mdec/netboot \
	    ${RELEASEDIR}/${MACHINE}/installation/netboot
	${RELEASE_INSTALL} ${DESTDIR}/usr/mdec/tapeboot \
	    ${.CURDIR}/MakeBootTape ${.CURDIR}/MakeInstTape \
	    ${RELEASEDIR}/${MACHINE}/installation/tapeimage

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
