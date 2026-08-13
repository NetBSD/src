#	$NetBSD: Makefile.inc,v 1.2 2026/08/13 03:51:21 thorpej Exp $
#
#	etc.hb68k/Makefile.inc -- hb68k-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=

.if ${MACHINE_ARCH} == "m68000"
KERNEL_SETS+=		PHAETHON1
.else
KERNEL_SETS+=		WRAP030
.endif

#EXTRA_KERNELS=		INSTALL

#BUILD_KERNELS=		RAMDISK

#INSTALLATION_DIRS+=	installation/miniroot	\
#			installation/netboot	\
#			installation/tapeimage
