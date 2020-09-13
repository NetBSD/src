#	$NetBSD: Makefile,v 1.19 2020/09/13 02:37:32 kamil Exp $

.include <bsd.own.mk>

# crtstuff is built out of elsewhere, or not at all
SUBDIR+= libgcc .WAIT
SUBDIR+= libiberty libobjc libgomp
SUBDIR+= libbacktrace
SUBDIR+= liblto_plugin

.if (${MKLIBSTDCXX} != "no") && (${MKCXX} != "no")
SUBDIR+= libsupc++ libstdc++-v3

SUBDIR+= .WAIT libasan liblsan libubsan

# TSan does not work with 32bit address space
.if (!empty(MACHINE_ARCH:M*64*) || ${MACHINE_ARCH} == alpha)
SUBDIR+= libtsan
.endif
.endif

.include <bsd.subdir.mk>
