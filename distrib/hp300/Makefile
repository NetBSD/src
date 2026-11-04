#	$NetBSD: Makefile,v 1.18.74.1 2026/11/04 22:36:53 perseant Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		ramdisk .WAIT instkernel cdroms
TARGETS+=	release

release: check_RELEASEDIR .WAIT HP-IB.geometry
	${RELEASE_INSTALL} ${.CURDIR}/HP-IB.geometry \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
