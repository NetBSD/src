#	$NetBSD: Makefile.inc,v 1.14 2002/12/02 13:18:08 lukem Exp $
#
#	mac68k-specific release building goo
#

KERNEL_SETS=		GENERIC GENERICSBC

BUILD_KERNELS=		INSTALL INSTALLSBC

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/instkernel

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/installation/instkernel '*.gz'
