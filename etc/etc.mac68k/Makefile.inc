#	$NetBSD: Makefile.inc,v 1.20 2008/03/18 04:20:37 lukem Exp $
#
#	etc.mac68k/Makefile.inc -- mac68k-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC GENERICSBC

BUILD_KERNELS=		INSTALL INSTALLSBC

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/instkernel

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/instkernel '*.gz'

# For "make iso-image"
# mkisofs arguments to create a hybrid (HFS/Rockridge) CD-ROM image.
MKISOFS_FLAGS+=	-hfs --macbin \
	-map ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc/extensions.map

iso-image: do-iso-image
