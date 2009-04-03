#	$NetBSD: Makefile,v 1.3 2009/04/03 22:36:34 perry Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

BOOT=	${DESTDIR}/usr/mdec/boot

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms
TARGETS+=	release

release: check_RELEASEDIR .WAIT ${BOOT}
	gzip -9nc ${BOOT} > ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/boot.gz

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
