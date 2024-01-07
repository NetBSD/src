#	$NetBSD: Makefile.inc,v 1.1 2024/01/07 00:58:11 thorpej Exp $
#
#	etc.virt68k/Makefile.inc -- virt68k-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

INSTALLATION_DIRS+=	installation/ramdisks

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/ramdisks -a
