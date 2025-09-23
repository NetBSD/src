#	$NetBSD: Makefile,v 1.6 2025/09/23 20:12:06 rillig Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

BOOT=	${DESTDIR}/usr/mdec/boot

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms
TARGETS+=	release

release: check_RELEASEDIR .WAIT ${BOOT}
	${TOOL_GZIP_N} -9c ${BOOT} \
	    > ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/boot.gz

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
