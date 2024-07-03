#	$NetBSD: Makefile.inc,v 1.23.10.2 2024/07/03 18:38:54 martin Exp $
#
#	etc.mac68k/Makefile.inc -- mac68k-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC GENERICSBC WSFB WSFBSBC

BUILD_KERNELS=		INSTALL INSTALLSBC

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/instkernel

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/instkernel '*.gz'

# For "make iso-image"
# mkisofs arguments to create a hybrid (HFS/Rockridge) CD-ROM image.
MKISOFS_FLAGS+=	-hfs --macbin \
	-map ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc/extensions.map
# exclude large debug sets to avoid possible 700MB overflow
MKISOFS_FLAGS+=	-m debug.tgz -m xdebug.tgz

iso-image: do-iso-image
