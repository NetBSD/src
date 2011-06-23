#	$NetBSD: Makefile,v 1.1.4.1 2011/06/23 14:18:18 cherry Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.if ${MACHINE_ARCH} == "sh3el"
SUBDIR=		instkernel
.endif
SUBDIR+=	cdroms
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
