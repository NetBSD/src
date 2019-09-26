#	$NetBSD: Makefile,v 1.9 2019/09/26 08:03:20 mrg Exp $

.include <bsd.own.mk>

# crtstuff is built out of elsewhere, or not at all
SUBDIR+= libgcc .WAIT
SUBDIR+= libiberty libobjc libgomp
SUBDIR+= libbacktrace
SUBDIR+= liblto_plugin

.if (${MKLIBSTDCXX} != "no") && (${MKCXX} != "no")
SUBDIR+= libsupc++ libstdc++-v3

SUBDIR+= .WAIT libasan liblsan libubsan
.endif

.include <bsd.subdir.mk>
