#	$NetBSD: Makefile.inc,v 1.6 2002/12/02 13:18:05 lukem Exp $

KERNEL_SETS=		GENERIC GENERIC.MP

BUILD_KERNELS+=		INSTALL

INSTALLATION_DIRS+=	installation/floppy	\
			installation/diskimage	\
			installation/instkernel	\
			installation/netboot	\
			installation/misc

snap_md_post:
	cd ${KERNSRCDIR}/arch/alpha/stand/netboot && ${MAKE} release
