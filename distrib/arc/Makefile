#	$NetBSD: Makefile,v 1.6.74.1 2026/11/04 22:36:51 perseant Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

BOOT=	${DESTDIR}/usr/mdec/boot

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms
TARGETS+=	release

release: check_RELEASEDIR .WAIT ${BOOT}
	${RELEASE_INSTALL} ${BOOT} ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
