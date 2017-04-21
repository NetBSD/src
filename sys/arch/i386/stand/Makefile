#	$NetBSD: Makefile,v 1.27.32.1 2017/04/21 16:53:28 bouyer Exp $

SUBDIR=		mbr boot
SUBDIR+=	bootxx cdboot fatboot
SUBDIR+=	dosboot misc
SUBDIR+=	pxeboot
.if ${MACHINE} == "amd64"
SUBDIR+=	efiboot
.endif

.include <bsd.subdir.mk>
