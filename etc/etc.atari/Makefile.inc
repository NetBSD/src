#	$NetBSD: Makefile.inc,v 1.14 2002/12/02 13:31:17 lukem Exp $
#
#	etc.atari/Makefile.inc -- atari-specific etc Makefile targets
#

KERNEL_SETS=		BOOT HADES MILAN-ISAIDE MILAN-PCIIDE

EXTRA_KERNELS=		ATARITT BOOTX FALCON

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/misc

snap_md_post:
#	${MAKESUMS} -t ${RELEASEDIR}/installation/miniroot '*.gz'
