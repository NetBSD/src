#	$NetBSD: Makefile,v 1.3.12.1 2008/06/02 13:21:13 mjf Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		instkernel stand cdroms

release: check_RELEASEDIR
	${MAKEDIRTARGET} instkernel release
	${MAKEDIRTARGET} stand release
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation '*.gz' '*.exe'

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
