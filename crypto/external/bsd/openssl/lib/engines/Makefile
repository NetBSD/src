#	$NetBSD: Makefile,v 1.1.14.1 2023/08/11 13:41:53 martin Exp $

.include "bsd.own.mk"

# OpenSSL engines.
SUBDIR= dasync devcrypto loader_attic ossltest

# VIA Padlock -- only makes sense for x86 which might be a VIA CPU
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=padlock
.endif

.include <bsd.subdir.mk>
