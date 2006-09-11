#	$NetBSD: Makefile,v 1.2 2006/09/11 06:12:23 riz Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.if ${MACHINE_ARCH} == "mipsel"
SUBDIR=		instkernel
.endif
TARGETS+=	release

.include <bsd.subdir.mk>
