#	$NetBSD: Makefile,v 1.3.2.1 2026/11/04 22:37:09 perseant Exp $

.include <bsd.own.mk>

CDBASE=		sparc64dvd		# gives ${CDBASE}.iso

.if ${MKKMOD} != "no"
CD_SETS+=	modules
.endif

.include "${.CURDIR}/../Makefile.cdrom"
