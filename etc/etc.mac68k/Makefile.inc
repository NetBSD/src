#	$NetBSD: Makefile.inc,v 1.17.10.1 2007/03/09 06:59:24 riz Exp $
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

iso-image: do-iso-image
