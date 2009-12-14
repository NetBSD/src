#	$NetBSD: Makefile.inc,v 1.17 2009/12/14 01:12:05 matt Exp $
#
#	etc.pmax/Makefile.inc -- pmax-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

.if ${MACHINE_ARCH} == "mipsel"

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL RAMDISK

.else

KERNEL_SETS=		GENERIC64

BUILD_KERNELS=		INSTALL64 RAMDISK64

.endif

INSTALLATION_DIRS+=	installation/diskimage	\
			installation/netboot \
			installation/miniroot
