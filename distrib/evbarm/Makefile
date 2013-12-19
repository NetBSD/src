#	$NetBSD: Makefile,v 1.8.26.1 2013/12/19 01:13:27 matt Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=
.if ${MACHINE_ARCH:Marmv7*} == ""
SUBDIR+=	gzboot
.endif
SUBDIR+=	instkernel cdroms
TARGETS+=	release


iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
