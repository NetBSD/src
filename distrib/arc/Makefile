#	Makefile,v 1.3 2007/03/06 21:56:54 bouyer Exp

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

BOOT=	${DESTDIR}/usr/mdec/boot

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms
TARGETS+=	release

release: check_RELEASEDIR .WAIT ${BOOT}
	${RELEASE_INSTALL} ${BOOT} ${RELEASEDIR}/${MACHINE}/installation

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
