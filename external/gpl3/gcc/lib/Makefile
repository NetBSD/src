#	$NetBSD: Makefile,v 1.7 2014/03/01 10:00:31 mrg Exp $

.include <bsd.own.mk>

# crtstuff is built out of elsewhere, or not at all
SUBDIR+= libgcc libiberty libobjc libgomp
SUBDIR+= libmudflap libmudflapth libbacktrace
SUBDIR+= liblto_plugin

.if (${MKLIBSTDCXX} != "no")
SUBDIR+= libsupc++ libstdc++-v3
.endif

.include <bsd.subdir.mk>
