#	$NetBSD: Makefile.inc,v 1.27 2026/11/09 23:48:41 nat Exp $
#
#	etc.mac68k/Makefile.inc -- mac68k-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		AUDIO AUDIOSBC GENERIC GENERICSBC WSFB WSFBSBC

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
