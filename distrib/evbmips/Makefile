#	$NetBSD: Makefile,v 1.2.2.1 2007/03/09 06:59:38 riz Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		cdroms
.if ${MACHINE_ARCH} == "mipsel"
SUBDIR+=	instkernel
.endif
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
