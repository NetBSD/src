#	$NetBSD: Makefile,v 1.5 1997/01/22 04:19:48 thorpej Exp $

SUBDIR+= lib usr.bin

.if (${MACHINE_ARCH} != "alpha")

.if exists(games)
SUBDIR+= games
.endif

SUBDIR+= libexec
.endif

.include <bsd.subdir.mk>
