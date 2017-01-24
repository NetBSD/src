#	$NetBSD: Makefile,v 1.28 2017/01/24 11:09:14 nonaka Exp $

SUBDIR=		mbr boot
SUBDIR+=	bootxx cdboot fatboot
SUBDIR+=	dosboot misc
SUBDIR+=	pxeboot
.if ${MACHINE} == "amd64"
SUBDIR+=	efiboot
.endif

.include <bsd.subdir.mk>
