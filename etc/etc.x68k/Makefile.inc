#	$NetBSD: Makefile.inc,v 1.17 2003/01/03 15:34:40 lukem Exp $
#
#	etc.x68k/Makefile.inc -- x68k-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/floppy \
			installation/misc

LOCALTIME=		Japan

snap_md_post:
	cd ${KERNSRCDIR}/arch/x68k/stand/loadbsd && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/floppy '*.gz'
