#	$NetBSD: Makefile.inc,v 1.10 2002/05/02 15:09:47 lukem Exp $
#
#	etc.pmax/Makefile.inc -- pmax-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL RAMDISK

INSTALLATION_DIRS+=	installation/diskimage	\
			installation/netboot

# location and name of the stripped down NFS install kernel
NFSCONF=		INSTALL
NFSKERNNAME=		nfsnetbsd
NFSKERNDIR=		${KERNOBJDIR}/${NFSCONF}
NFSKERN=		${NFSKERNDIR}/${NFSKERNNAME}

snap_md_post: ${DISKIMAGE} ${DISKIMAGE_TAR} ${INSTALLKERN}
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${BINMODE} \
	    	${NFSKERN}.ecoff ${RELEASEDIR}/binary/kernel/${NFSKERNNAME}
	gzip -9 ${RELEASEDIR}/binary/kernel/${NFSKERNNAME}
	cd ${.CURDIR}/../distrib/pmax && \
	    ${MAKE} depend && ${MAKE} ${_J} && ${MAKE} install
