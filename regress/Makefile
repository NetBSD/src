#	$NetBSD: Makefile,v 1.9 1999/09/26 10:22:29 mrg Exp $

SRCTOP=	..
.include <bsd.crypto.mk>

# missing: bin libexec sbin usr.sbin share games gnu
SUBDIR+= include lib sys usr.bin

.if defined(CRYPTOPATH)
.sinclude "${CRYPTOPATH}/regress/Makefile.frag"
.endif

.include <bsd.subdir.mk>
