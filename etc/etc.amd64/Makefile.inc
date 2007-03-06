#	$NetBSD: Makefile.inc,v 1.7 2007/03/06 21:47:46 bouyer Exp $
#
#	etc.amd64/Makefile.inc -- amd64-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC.MP

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+= 	installation/cdrom
INSTALLATION_DIRS+= 	installation/floppy
INSTALLATION_DIRS+= 	installation/misc

snap_md_post:
	cd ${KERNSRCDIR}/arch/i386/stand/pxeboot && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/cdrom '*.iso'
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/floppy '*.fs'
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/misc '*.*'
