#	$NetBSD: Makefile.inc,v 1.18 2003/01/03 15:34:38 lukem Exp $
#
#	etc.hp300/Makefile.inc -- hp300-specific etc Makefile targets
#

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
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/misc 'SYS*'
