#	$NetBSD: Makefile,v 1.22 2023/08/01 05:57:56 mrg Exp $

.include <bsd.own.mk>

# crtstuff is built out of elsewhere, or not at all
SUBDIR+= libgcc .WAIT
SUBDIR+= libiberty libobjc libgomp
SUBDIR+= libbacktrace
SUBDIR+= liblto_plugin

.if (${MKLIBSTDCXX} != "no") && (${MKCXX} != "no")
SUBDIR+= libsupc++ libstdc++-v3

# XXX needs re-port
.if 0
SUBDIR+= .WAIT libasan liblsan libubsan

# TSan does not work with 32bit address space and has not been ported
# to all 64-bit architectures.
.if ${MACHINE} == "amd64"
SUBDIR+= libtsan
.endif
.endif # 0

.endif

.include <bsd.subdir.mk>
