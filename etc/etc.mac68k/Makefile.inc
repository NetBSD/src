#	$NetBSD: Makefile.inc,v 1.16 2003/01/03 15:34:38 lukem Exp $
#
#	etc.mac68k/Makefile.inc -- mac68k-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERICSBC

BUILD_KERNELS=		INSTALL INSTALLSBC

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/instkernel

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/instkernel '*.gz'
