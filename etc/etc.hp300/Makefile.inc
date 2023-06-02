#	$NetBSD: Makefile.inc,v 1.22 2023/06/02 14:25:35 lukem Exp $
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
.if ${MKUPDATE} == "no"
	cd ${KERNSRCDIR}/arch/hp300/stand && ${MAKE} cleandir
.endif
	cd ${KERNSRCDIR}/arch/hp300/stand && \
	    ${MAKE} depend && ${MAKE} && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/miniroot '*.fs*'
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc 'SYS*'
