#	$NetBSD: Makefile.inc,v 1.15 2002/12/02 13:31:19 lukem Exp $
#
#	etc.mac68k/Makefile.inc -- mac68k-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERICSBC

BUILD_KERNELS=		INSTALL INSTALLSBC

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/instkernel

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/installation/instkernel '*.gz'
