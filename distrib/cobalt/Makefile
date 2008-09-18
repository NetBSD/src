#	$NetBSD: Makefile,v 1.1.12.1 2008/09/18 04:41:09 wrstuden Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

BOOT=	${DESTDIR}/usr/mdec/boot

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms
TARGETS+=	release

release: check_RELEASEDIR .WAIT ${BOOT}
	gzip -9c ${BOOT} > ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/boot.gz

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
