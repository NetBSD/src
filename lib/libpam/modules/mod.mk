#	$NetBSD: mod.mk,v 1.18 2024/04/05 01:16:00 christos Exp $

WARNS=6
LIBISMODULE=yes
MAKESTATICLIB=yes
MAKELINKLIB=yes
LINKINSTALL=no

.include <bsd.own.mk>

.include "${.PARSEDIR}/../Makefile.inc"

.if defined(MLIBDIR)
LIBDIR=/usr/lib/${MLIBDIR}/security
.else
LIBDIR=/usr/lib/security
.endif

.include <bsd.lib.mk>
