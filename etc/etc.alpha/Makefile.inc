#	$NetBSD: Makefile.inc,v 1.8.18.2 2007/07/24 10:49:24 pavel Exp $
#
#	etc.alpha/Makefile.inc -- alpha-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC GENERIC.MP

BUILD_KERNELS+=		INSTALL

INSTALLATION_DIRS+=	installation/floppy	\
			installation/diskimage	\
			installation/instkernel	\
			installation/netboot	\
			installation/misc

snap_md_post:
	cd ${KERNSRCDIR}/arch/alpha/stand/netboot && ${MAKE} release
