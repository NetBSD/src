#	$NetBSD: Makefile,v 1.6.12.2 2008/06/29 08:25:12 mjf Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		instkernel stand cdroms

release: check_RELEASEDIR
	${MAKEDIRTARGET} instkernel release
	${MAKEDIRTARGET} stand release
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation '*.gz' '*.exe'

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
