#	$NetBSD: Makefile,v 1.3 2025/09/23 20:12:06 rillig Exp $

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
