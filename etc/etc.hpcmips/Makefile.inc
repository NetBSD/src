#	$NetBSD: Makefile.inc,v 1.8 2002/12/02 13:31:19 lukem Exp $
#
#	etc.hpcmips/Makefile.inc -- hpcmips-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC TX3912

BUILD_KERNELS=		RAMDISK INSTALL_TX3912

INSTALLATION_DIRS+=	installation/miniroot
