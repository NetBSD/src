#	$NetBSD: Makefile,v 1.29.2.1 2008/09/18 04:41:09 wrstuden Exp $

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
