#	$NetBSD: Makefile,v 1.5 2009/12/14 01:12:05 matt Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		cdroms
SUBDIR+=	instkernel
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
