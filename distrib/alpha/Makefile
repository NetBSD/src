#	$NetBSD: Makefile,v 1.23.74.2 2026/11/04 22:36:50 perseant Exp $

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

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

live_image: .MAKE
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
