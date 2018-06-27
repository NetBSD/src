#	$NetBSD: Makefile,v 1.15 2018/06/27 06:06:39 martin Exp $

.include <bsd.own.mk>

# crtstuff is built out of elsewhere, or not at all
SUBDIR+= libgcc .WAIT
SUBDIR+= libiberty libobjc libgomp
SUBDIR+= libbacktrace
# SUBDIR+= liblsan
SUBDIR+= liblto_plugin

.if (${MKLIBSTDCXX} != "no") && (${MKCXX} != "no")
SUBDIR+= libsupc++ libstdc++-v3
.endif

SUBDIR+= .WAIT libasan libubsan

.include <bsd.subdir.mk>
