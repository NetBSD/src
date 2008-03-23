#	$NetBSD: Makefile,v 1.1.8.2 2008/03/23 00:21:01 matt Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

BOOT=	${DESTDIR}/usr/mdec/boot

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms
TARGETS+=	release

release: check_RELEASEDIR .WAIT ${BOOT}
	gzip -9c ${BOOT} > ${RELEASEDIR}/${MACHINE}/installation/boot.gz

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
