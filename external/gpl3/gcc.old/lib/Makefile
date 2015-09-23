#	$NetBSD: Makefile,v 1.3 2015/09/23 03:39:29 mrg Exp $

.include <bsd.own.mk>

# crtstuff is built out of elsewhere, or not at all
SUBDIR+= libgcc libiberty libobjc libgomp
SUBDIR+= libmudflap libmudflapth libbacktrace
SUBDIR+= liblto_plugin

.if (${MKLIBSTDCXX} != "no")
SUBDIR+= libsupc++ libstdc++-v3
.endif

SUBDIR+= libasan

.include <bsd.subdir.mk>
