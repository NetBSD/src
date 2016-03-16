#	$NetBSD: Makefile,v 1.10 2016/03/16 05:41:47 mrg Exp $

.include <bsd.own.mk>

# crtstuff is built out of elsewhere, or not at all
SUBDIR+= libgcc .WAIT
SUBDIR+= libiberty libobjc libgomp
SUBDIR+= libbacktrace
SUBDIR+= liblto_plugin

.if (${MKLIBSTDCXX} != "no") && (${MKCXX} != "no")
SUBDIR+= libsupc++ libstdc++-v3
.endif

# XXX GCC5
#SUBDIR+= libasan

.include <bsd.subdir.mk>
