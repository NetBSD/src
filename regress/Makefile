#	$NetBSD: Makefile,v 1.8.4.1 1999/12/27 18:30:18 wrstuden Exp $

SRCTOP=	..
.include <bsd.crypto.mk>

# missing: bin libexec sbin usr.sbin share games gnu
SUBDIR+= include lib sys usr.bin

.if defined(CRYPTOPATH)
.sinclude "${CRYPTOPATH}/regress/Makefile.frag"
.endif

.include <bsd.subdir.mk>
