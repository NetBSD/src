#	$NetBSD: Makefile,v 1.3 2007/03/06 21:56:59 bouyer Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.if ${MACHINE_ARCH} == "mipsel"
SUBDIR=		instkernel
.endif
TARGETS+=	release cdroms

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
