# $NetBSD: Makefile.inc,v 1.1 2016/08/05 16:22:08 scole Exp $
#
#	etc.ia64/Makefile.inc -- ia64-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

# XXX fix all this

KERNEL_SETS=	GENERIC
KERNEL_SETS+=	GENERIC.SKI

#BUILD_KERNELS+=	INSTALL

#INSTALLATION_DIRS+=	installation/cdrom
#INSTALLATION_DIRS+=	installation/diskimage
#INSTALLATION_DIRS+=	installation/instkernel
#INSTALLATION_DIRS+=	installation/netboot

#snap_md_post:
#	cd ${KERNSRCDIR}/arch/i386/stand/dosboot && ${MAKE} release
#	cd ${KERNSRCDIR}/arch/i386/stand/pxeboot && ${MAKE} release
#	cd ${KERNSRCDIR}/arch/i386/stand/misc && ${MAKE} release
#	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/cdrom '*.iso'
#	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/floppy '*.fs'
#	cd ${KERNSRCDIR}/arch/alpha/stand/netboot && ${MAKE} release
