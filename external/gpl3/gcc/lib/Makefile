#	$NetBSD: Makefile,v 1.7.4.1 2015/01/25 00:19:33 martin Exp $

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
