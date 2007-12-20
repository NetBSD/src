#	$NetBSD: Makefile.inc,v 1.10.2.2 2007/12/20 23:32:25 ad Exp $
#
#	etc.amd64/Makefile.inc -- amd64-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC
KERNEL_SETS+=		XEN3_DOM0 XEN3_DOMU

BUILD_KERNELS=		INSTALL INSTALL_XEN3_DOMU

INSTALLATION_DIRS+= 	installation/cdrom
INSTALLATION_DIRS+= 	installation/floppy
INSTALLATION_DIRS+= 	installation/misc

snap_md_post:
	cd ${KERNSRCDIR}/arch/i386/stand/pxeboot && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/cdrom '*.iso'
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/floppy '*.fs'
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/misc '*.*'
