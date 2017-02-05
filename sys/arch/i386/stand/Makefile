#	$NetBSD: Makefile,v 1.27.24.1 2017/02/05 13:40:12 skrll Exp $

SUBDIR=		mbr boot
SUBDIR+=	bootxx cdboot fatboot
SUBDIR+=	dosboot misc
SUBDIR+=	pxeboot
.if ${MACHINE} == "amd64"
SUBDIR+=	efiboot
.endif

.include <bsd.subdir.mk>
