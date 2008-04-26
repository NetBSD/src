#	$NetBSD: Makefile,v 1.29 2008/04/26 14:49:43 tsutsui Exp $

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
