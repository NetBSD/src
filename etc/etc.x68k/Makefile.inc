#	$NetBSD: Makefile.inc,v 1.18.14.2 2007/07/24 10:49:29 pavel Exp $
#
#	etc.x68k/Makefile.inc -- x68k-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/floppy \
			installation/misc

LOCALTIME=		Japan

snap_md_post:
	cd ${KERNSRCDIR}/arch/x68k/stand/loadbsd && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/floppy '*.gz'
