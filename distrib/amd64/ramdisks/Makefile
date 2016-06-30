#	$NetBSD: Makefile,v 1.3 2016/06/30 12:56:27 pgoyette Exp $

SUBDIR=
SUBDIR+=	ramdisk

.if ${MKCRYPTO:Uyes} != "no"
SUBDIR+=	ramdisk-cgdroot
.endif

TARGETS+=	release

.include <bsd.subdir.mk>
