#	$NetBSD: Makefile,v 1.6 1997/07/21 04:48:06 mrg Exp $

SUBDIR+= lib usr.bin

.if exists(games)
SUBDIR+= games
.endif

.if (${MACHINE_ARCH} != "alpha")
SUBDIR+= libexec
.endif

.include <bsd.subdir.mk>
