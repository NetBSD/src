#	$NetBSD: Makefile.inc,v 1.10 2020/09/19 03:29:57 thorpej Exp $
#
#	etc.alpha/Makefile.inc -- alpha-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS+=		INSTALL

INSTALLATION_DIRS+=	installation/floppy	\
			installation/diskimage	\
			installation/instkernel	\
			installation/netboot

snap_md_post:
	cd ${KERNSRCDIR}/arch/alpha/stand/netboot && ${MAKE} release
