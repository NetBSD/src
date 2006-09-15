#	$NetBSD: Makefile,v 1.1.2.1 2006/09/15 12:05:19 tron Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.if ${MACHINE_ARCH} == "mipsel"
SUBDIR=		instkernel
.endif
TARGETS+=	release

.include <bsd.subdir.mk>
