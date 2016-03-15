#	$NetBSD: Makefile,v 1.9 2016/03/15 03:19:38 mrg Exp $

.include <bsd.own.mk>

# crtstuff is built out of elsewhere, or not at all
SUBDIR+= libgcc libiberty libobjc libgomp
SUBDIR+= libbacktrace
SUBDIR+= liblto_plugin

.if (${MKLIBSTDCXX} != "no")
SUBDIR+= libsupc++ libstdc++-v3
.endif

SUBDIR+= libasan

.include <bsd.subdir.mk>
