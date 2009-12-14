# $NetBSD: Makefile.inc,v 1.4 2009/12/14 01:12:05 matt Exp $
#
#	etc.sbmips/Makefile.inc -- sbmips-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC${MACHINE_ARCH:S/mips//:C/e[lb]//}
