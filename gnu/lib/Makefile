#	$NetBSD: Makefile,v 1.14 1998/07/29 19:04:16 thorpej Exp $

# XXX Temporary for USE_EGCS
.include <bsd.own.mk>

SUBDIR+= libmalloc
.ifdef USE_EGCS
SUBDIR+= libf2c libstdc++
.else
SUBDIR+= libg++
.endif

# XXX Until our bintuils is upgraded
.if (${MACHINE_ARCH} != "powerpc")
SUBDIR+= bfd
.endif

.include <bsd.subdir.mk>
