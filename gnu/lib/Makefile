#	$NetBSD: Makefile,v 1.18 1999/02/02 17:45:02 tv Exp $

SUBDIR+= libg2c libgcc libmalloc libobjc libstdc++

# XXX Until our bintuils is upgraded
.if (${MACHINE_ARCH} != "powerpc")
# SUBDIR+= bfd
.endif

.include <bsd.subdir.mk>
