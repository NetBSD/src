#	$NetBSD: Makefile.inc,v 1.16.2.1 2004/06/27 13:49:42 he Exp $
#
#	etc.mac68k/Makefile.inc -- mac68k-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERICSBC

BUILD_KERNELS=		INSTALL INSTALLSBC

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/instkernel

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/instkernel '*.gz'

# For "make iso-image"
# mkisofs arguments to create a hybrid (HFS/Rockridge) CD-ROM image.
MKISOFS_FLAGS+=	-hfs --macbin \
	-map ${RELEASEDIR}/${MACHINE}/installation/misc/extensions.map
