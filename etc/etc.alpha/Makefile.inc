#	$NetBSD: Makefile.inc,v 1.8.58.1 2015/02/27 10:10:32 martin Exp $
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
			installation/netboot

snap_md_post:
	cd ${KERNSRCDIR}/arch/alpha/stand/netboot && ${MAKE} release
