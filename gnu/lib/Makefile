#	$NetBSD: Makefile,v 1.15 1998/08/16 17:16:00 tv Exp $

# XXX Temporary for USE_EGCS
.include <bsd.own.mk>

SUBDIR+= libmalloc
.ifdef USE_EGCS
#SUBDIR+= libg2c libstdc++		- currently in flux
.else
SUBDIR+= libg++
.endif

# XXX Until our bintuils is upgraded
.if (${MACHINE_ARCH} != "powerpc")
SUBDIR+= bfd
.endif

.include <bsd.subdir.mk>
