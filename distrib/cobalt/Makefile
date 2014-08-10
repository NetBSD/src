#	$NetBSD: Makefile,v 1.4.2.1 2014/08/10 06:47:54 tls Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

BOOT=	${DESTDIR}/usr/mdec/boot

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms
TARGETS+=	release

release: check_RELEASEDIR .WAIT ${BOOT}
	${TOOL_GZIP_N} -9c ${BOOT} \
	    > ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/boot.gz

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
