#	$NetBSD: Makefile,v 1.6 2013/04/28 06:14:52 joerg Exp $

.include <bsd.own.mk>

# crtstuff is built out of elsewhere, or not at all
SUBDIR+= libgcc libiberty libobjc libgomp
SUBDIR+= libmudflap libmudflapth

.if (${MKLIBSTDCXX} != "no")
SUBDIR+= libsupc++ libstdc++-v3
.endif

.include <bsd.subdir.mk>
