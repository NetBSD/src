#	$NetBSD: Makefile,v 1.17 1998/12/12 23:50:45 tv Exp $

SUBDIR+= libg2c libgcc libmalloc libobjc libstdc++

# XXX Until our bintuils is upgraded
.if (${MACHINE_ARCH} != "powerpc")
SUBDIR+= bfd
.endif

.include <bsd.subdir.mk>
