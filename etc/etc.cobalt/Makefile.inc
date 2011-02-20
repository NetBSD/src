#	$NetBSD: Makefile.inc,v 1.5 2011/02/20 08:27:32 matt Exp $
#
#	etc.cobalt/Makefile.inc -- cobalt-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

.if ${MACHINE_ARCH} == "mips64el"
KERNEL_SETS=		GENERIC32 GENERIC64 INSTALL32 INSTALL64

BUILD_KERNELS=		RAMDISK32 RAMDISK64
.else
KERNEL_SETS=		GENERIC INSTALL

BUILD_KERNELS=		RAMDISK
.endif
