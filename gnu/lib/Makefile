#	$NetBSD: Makefile,v 1.16 1998/08/28 15:34:24 tv Exp $

# XXX Temporary for USE_EGCS
.include <bsd.own.mk>

SUBDIR+= libmalloc
.ifdef USE_EGCS
SUBDIR+= libg2c libgcc libobjc libstdc++
.else
SUBDIR+= libg++
.endif

# XXX Until our bintuils is upgraded
.if (${MACHINE_ARCH} != "powerpc")
SUBDIR+= bfd
.endif

.include <bsd.subdir.mk>
