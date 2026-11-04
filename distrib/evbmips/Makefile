#	$NetBSD: Makefile,v 1.5.60.1 2026/11/04 22:36:53 perseant Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		cdroms
SUBDIR+=	instkernel
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
