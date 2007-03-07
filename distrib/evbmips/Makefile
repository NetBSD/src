#	$NetBSD: Makefile,v 1.4 2007/03/07 19:42:02 bouyer Exp $

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
