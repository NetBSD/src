#	$NetBSD: Makefile,v 1.27.28.1 2017/03/20 06:57:15 pgoyette Exp $

SUBDIR=		mbr boot
SUBDIR+=	bootxx cdboot fatboot
SUBDIR+=	dosboot misc
SUBDIR+=	pxeboot
.if ${MACHINE} == "amd64"
SUBDIR+=	efiboot
.endif

.include <bsd.subdir.mk>
