#	$NetBSD: Makefile,v 1.20 2020/09/15 04:55:23 martin Exp $

.include <bsd.own.mk>

# crtstuff is built out of elsewhere, or not at all
SUBDIR+= libgcc .WAIT
SUBDIR+= libiberty libobjc libgomp
SUBDIR+= libbacktrace
SUBDIR+= liblto_plugin

.if (${MKLIBSTDCXX} != "no") && (${MKCXX} != "no")
SUBDIR+= libsupc++ libstdc++-v3

SUBDIR+= .WAIT libasan liblsan libubsan

# TSan does not work with 32bit address space and has not been porter
# to all architectures
.if (!empty(MACHINE_ARCH:M*64*) || ${MACHINE_ARCH} == alpha) && \
	${MACHINE_ARCH} != "sparc64"
SUBDIR+= libtsan
.endif
.endif

.include <bsd.subdir.mk>
