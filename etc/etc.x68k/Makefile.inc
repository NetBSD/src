#	$NetBSD: Makefile.inc,v 1.16 2002/12/02 13:18:15 lukem Exp $
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
	${MAKESUMS} -t ${RELEASEDIR}/installation/floppy '*.gz'
