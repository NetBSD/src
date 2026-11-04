#	$NetBSD: Makefile,v 1.2.56.1 2026/11/04 22:36:53 perseant Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.if ${MACHINE_ARCH} == "sh3el"
SUBDIR=		instkernel
.endif
SUBDIR+=	cdroms
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
