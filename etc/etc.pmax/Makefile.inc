#	$NetBSD: Makefile.inc,v 1.18 2016/11/04 19:18:50 flxd Exp $
#
#	etc.pmax/Makefile.inc -- pmax-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

.if ${MACHINE_ARCH} == "mipsel"

KERNEL_SETS=		GENERIC GENERIC_USB

BUILD_KERNELS=		INSTALL RAMDISK

.else

KERNEL_SETS=		GENERIC64 GENERIC64_USB

BUILD_KERNELS=		INSTALL64 RAMDISK64

.endif

INSTALLATION_DIRS+=	installation/diskimage	\
			installation/netboot \
			installation/miniroot
