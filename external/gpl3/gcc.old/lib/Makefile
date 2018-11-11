#	$NetBSD: Makefile,v 1.7 2018/11/11 22:49:47 mrg Exp $

.include <bsd.own.mk>

# crtstuff is built out of elsewhere, or not at all
SUBDIR+= libgcc .WAIT
SUBDIR+= libiberty libobjc libgomp
SUBDIR+= libbacktrace
SUBDIR+= liblto_plugin

.if (${MKLIBSTDCXX} != "no") && (${MKCXX} != "no")
SUBDIR+= libsupc++ libstdc++-v3
.endif

SUBDIR+= .WAIT libasan liblsan libubsan

.include <bsd.subdir.mk>
