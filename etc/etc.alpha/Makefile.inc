#	$NetBSD: Makefile.inc,v 1.7 2002/12/02 13:31:17 lukem Exp $
#
#	etc.alpha/Makefile.inc -- alpha-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC.MP

BUILD_KERNELS+=		INSTALL

INSTALLATION_DIRS+=	installation/floppy	\
			installation/diskimage	\
			installation/instkernel	\
			installation/netboot	\
			installation/misc

snap_md_post:
	cd ${KERNSRCDIR}/arch/alpha/stand/netboot && ${MAKE} release
