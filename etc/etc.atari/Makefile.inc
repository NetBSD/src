#	$NetBSD: Makefile.inc,v 1.15 2003/01/03 15:34:37 lukem Exp $
#
#	etc.atari/Makefile.inc -- atari-specific etc Makefile targets
#

KERNEL_SETS=		BOOT HADES MILAN-ISAIDE MILAN-PCIIDE

EXTRA_KERNELS=		ATARITT BOOTX FALCON

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/misc

snap_md_post:
#	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/miniroot '*.gz'
