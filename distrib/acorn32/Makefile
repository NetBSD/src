# $NetBSD: Makefile,v 1.20.12.1 2026/11/04 22:36:50 perseant Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		ramdisk .WAIT kernels stand .WAIT cdroms
TARGETS+=	release

release: check_RELEASEDIR
	${RELEASE_INSTALL} ${DISTRIBDIR}/notes/acorn32/prep.RISCOS \
		    ${RELEASEDIR}/${RELEASEMACHINEDIR}/

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
