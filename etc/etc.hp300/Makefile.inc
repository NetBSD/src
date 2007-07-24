#	$NetBSD: Makefile.inc,v 1.20 2007/07/24 10:49:25 pavel Exp $
#
#	etc.hp300/Makefile.inc -- hp300-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL RAMDISK

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/misc

snap_md_post:
.ifndef UPDATE
	cd ${KERNSRCDIR}/arch/hp300/stand && ${MAKE} cleandir
.endif
	cd ${KERNSRCDIR}/arch/hp300/stand && \
	    ${MAKE} depend && ${MAKE} && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/miniroot '*.fs*'
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/misc 'SYS*'
