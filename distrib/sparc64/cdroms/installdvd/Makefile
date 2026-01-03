#	$NetBSD: Makefile,v 1.4 2026/01/03 20:23:20 nia Exp $

.include <bsd.own.mk>

CDBASE=		sparc64dvd		# gives ${CDBASE}.iso

.if ${MKKMOD} != "no"
CD_SETS+=	modules
.endif

.include "${.CURDIR}/../Makefile.cdrom"
