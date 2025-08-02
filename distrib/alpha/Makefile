#	$NetBSD: Makefile,v 1.23.74.1 2025/08/02 05:20:18 perseant Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		instkernel cdroms # floppy-GENERIC
SUBDIR+=	.WAIT
SUBDIR+=	liveimage
TARGETS+=	release

release: check_RELEASEDIR .WAIT README.files
.for i in floppy diskimage instkernel
	${RELEASE_INSTALL} ${.CURDIR}/README.files \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/$i
.endfor

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

live_image:
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
