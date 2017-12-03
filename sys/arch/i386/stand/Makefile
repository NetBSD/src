#	$NetBSD: Makefile,v 1.27.6.1 2017/12/03 11:36:18 jdolecek Exp $

SUBDIR=		mbr boot
SUBDIR+=	bootxx cdboot fatboot
SUBDIR+=	dosboot misc
SUBDIR+=	pxeboot
.if ${MACHINE} == "amd64"
SUBDIR+=	efiboot
.endif

.include <bsd.subdir.mk>
