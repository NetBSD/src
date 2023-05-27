#	$NetBSD: Makefile,v 1.6 2023/05/27 02:28:45 riastradh Exp $

.include "bsd.own.mk"

# OpenSSL engines.
SUBDIR= dasync devcrypto loader_attic ossltest

# VIA Padlock -- only makes sense for x86 which might be a VIA CPU
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=padlock
.endif

.include <bsd.subdir.mk>
