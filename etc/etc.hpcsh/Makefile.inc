#	$NetBSD: Makefile.inc,v 1.3 2002/12/06 09:09:06 jdolecek Exp $
#
#	etc.hpcsh/Makefile.inc -- hpcsh-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC HPW650PA

BUILD_KERNELS=		RAMDISK

INSTALLATION_DIRS+=	installation/miniroot
