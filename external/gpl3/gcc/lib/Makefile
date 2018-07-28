#	$NetBSD: Makefile,v 1.13.10.1 2018/07/28 04:37:17 pgoyette Exp $

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
