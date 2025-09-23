#	$NetBSD: Makefile,v 1.7 2025/09/23 20:12:05 rillig Exp $

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
