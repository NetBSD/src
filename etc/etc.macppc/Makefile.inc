#	$NetBSD: Makefile.inc,v 1.12 2020/09/15 08:14:31 mrg Exp $
#
#	etc.macppc/Makefile.inc -- macppc-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

.if ${MACHINE_ARCH} != "powerpc64"
KERNEL_SETS=		GENERIC GENERIC.MP GENERIC_601 POWERMAC_G5 \
			POWERMAC_G5.MP

BUILD_KERNELS+=		INSTALL INSTALL_601 GENERIC_MD

INSTALLATION_DIRS+=	installation/floppy
.endif
