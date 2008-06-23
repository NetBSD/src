# $NetBSD: Makefile,v 1.19 2008/06/23 02:16:53 matt Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		ramdisk .WAIT kernels stand cdroms
TARGETS+=	release

release: check_RELEASEDIR
	${RELEASE_INSTALL} ${DISTRIBDIR}/notes/acorn32/prep.RISCOS \
		    ${RELEASEDIR}/${RELEASEMACHINEDIR}/

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
