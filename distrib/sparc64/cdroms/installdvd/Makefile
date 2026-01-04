#	$NetBSD: Makefile,v 1.3.4.1 2026/01/04 12:09:39 martin Exp $

.include <bsd.own.mk>

CDBASE=		sparc64dvd		# gives ${CDBASE}.iso

.if ${MKKMOD} != "no"
CD_SETS+=	modules
.endif

.include "${.CURDIR}/../Makefile.cdrom"
