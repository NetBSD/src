#	$NetBSD: Makefile,v 1.27.4.2 2008/09/22 18:14:43 bouyer Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		instkernel stand cdroms
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
