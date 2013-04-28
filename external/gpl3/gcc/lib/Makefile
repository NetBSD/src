#	$NetBSD: Makefile,v 1.5 2013/04/28 04:05:41 joerg Exp $

# crtstuff is built out of elsewhere, or not at all
SUBDIR+= libgcc libiberty libobjc libgomp
SUBDIR+= libmudflap libmudflapth

.if (${MKLIBSTDCXX} != "no")
SUBDIR+= libsupc++ libstdc++-v3
.endif

.include <bsd.subdir.mk>
