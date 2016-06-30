#	$NetBSD: Makefile,v 1.5 2016/06/30 12:56:27 pgoyette Exp $

SUBDIR=
SUBDIR+=	ramdisk-big

.if ${MKCRYPTO:Uyes} != "no"
SUBDIR+=	ramdisk-cgdroot
.endif

TARGETS+=	release

.include <bsd.subdir.mk>
