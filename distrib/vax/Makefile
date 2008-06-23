#	$NetBSD: Makefile,v 1.18 2008/06/23 02:16:54 matt Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		ramdisk .WAIT ramdisk-kernel .WAIT floppy cdroms
TARGETS+=	release

release: check_RELEASEDIR .WAIT ${DESTDIR}/usr/mdec/boot
	${RELEASE_INSTALL} ${DESTDIR}/usr/mdec/boot \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/netboot

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
