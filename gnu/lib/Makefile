#	$NetBSD: Makefile,v 1.9.2.1 1997/10/24 00:38:04 mellon Exp $

SUBDIR+= libmalloc
SUBDIR+= libg++

.if (${MACHINE_ARCH} != "alpha")
SUBDIR+= bfd readline
.endif

.include <bsd.subdir.mk>
