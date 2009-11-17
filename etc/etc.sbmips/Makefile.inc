# $NetBSD: Makefile.inc,v 1.3.26.1 2009/11/17 19:24:35 matt Exp $
#
#	etc.sbmips/Makefile.inc -- sbmips-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC${MACHINE_ARCH:S/mips//:C/e[lb]//}
