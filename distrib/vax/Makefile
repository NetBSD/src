#	$NetBSD: Makefile,v 1.19.54.1 2025/08/02 05:20:40 perseant Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		ramdisk .WAIT ramdisk-kernel .WAIT floppy cdroms liveimage
TARGETS+=	release

release: check_RELEASEDIR .WAIT ${DESTDIR}/usr/mdec/boot \
                                ${DESTDIR}/usr/mdec/boot.mop
	${RELEASE_INSTALL} ${DESTDIR}/usr/mdec/boot \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/netboot
	${RELEASE_INSTALL} ${DESTDIR}/usr/mdec/boot.mop \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/netboot

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

live_image:
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
