#	$NetBSD: Makefile,v 1.10 1997/10/23 18:15:04 cjs Exp $

SUBDIR+= libmalloc
SUBDIR+= libg++

.if (${MACHINE_ARCH} != "alpha")
SUBDIR+= bfd readline
.endif

.include <bsd.subdir.mk>
