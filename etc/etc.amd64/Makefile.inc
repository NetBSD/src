#	$NetBSD: Makefile.inc,v 1.14 2012/07/04 13:54:20 reinoud Exp $
#
#	etc.amd64/Makefile.inc -- amd64-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC
KERNEL_SETS+=		XEN3_DOM0 XEN3_DOMU
# KERNEL_SETS+=		GENERIC_USERMODE

BUILD_KERNELS=		INSTALL INSTALL_XEN3_DOMU

INSTALLATION_DIRS+= 	installation/cdrom
INSTALLATION_DIRS+= 	installation/floppy
INSTALLATION_DIRS+= 	installation/misc
INSTALLATION_DIRS+= 	installation/miniroot

snap_md_post:
	cd ${KERNSRCDIR}/arch/i386/stand/pxeboot && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/cdrom '*.iso'
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/floppy '*.fs'
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/miniroot '*.*'
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc '*.*'
