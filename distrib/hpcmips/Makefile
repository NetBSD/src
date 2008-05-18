#	$NetBSD: Makefile,v 1.28.14.1 2008/05/18 12:29:35 yamt Exp $

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
