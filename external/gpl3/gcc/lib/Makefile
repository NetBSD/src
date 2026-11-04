#	$NetBSD: Makefile,v 1.23.2.2 2026/11/04 22:52:23 perseant Exp $

.include <bsd.own.mk>

# crtstuff is built out of elsewhere, or not at all
SUBDIR+= libgcc .WAIT
SUBDIR+= libiberty libobjc libgomp
SUBDIR+= libbacktrace
SUBDIR+= liblto_plugin

.if (${MKLIBSTDCXX} != "no") && (${MKCXX} != "no")
SUBDIR+= libsupc++ libstdc++-v3

.if ${MKGCCCMDS} != "no"
SUBDIR+= .WAIT libasan liblsan libubsan

# TSan does not work with 32bit address space and has not been ported
# to all 64-bit architectures.
.if ${MACHINE} == "amd64" || ${MACHINE_CPU} == "aarch64"
SUBDIR+= libtsan
.endif
.endif

.endif

# If libgfortran needs libquadmath, build it first.
.if ${HAVE_QUADMATH} != no
SUBDIR+= libquadmath .WAIT
.endif
SUBDIR+= libgfortran

.include <bsd.subdir.mk>
