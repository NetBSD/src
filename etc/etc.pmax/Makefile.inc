#	$NetBSD: Makefile.inc,v 1.6.4.1 2001/09/30 11:57:19 he Exp $
#
#	etc.pmax/Makefile.inc -- pmax-specific etc Makefile targets
#

# Set this for any kernels beyond GENERIC you want to include
# in the distribution.
#EXTRA_KERNELS+=

# Set this for any kernels beyond GENERIC and ${EXTRA_KERNELS} you
# need to build to make installation tools/media.
BUILD_KERNELS+=INSTALL RAMDISK

# Add any extra release directories to be made
INSTALLATION_DIRS+=installation/diskimage installation/netboot

# location and name of the stripped down NFS install kernel
NFSCONF=		INSTALL
NFSKERNNAME=		nfsnetbsd
NFSKERNDIR=		${KERNOBJDIR}/${NFSCONF}
NFSKERN=		${NFSKERNDIR}/${NFSKERNNAME}

snap_md_post: ${DISKIMAGE} ${DISKIMAGE_TAR} ${INSTALLKERN}
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -o root -g wheel \
		-m ${BINMODE} ${NFSKERN}.ecoff \
		${RELEASEDIR}/binary/kernel/${NFSKERNNAME}
	gzip -9 ${RELEASEDIR}/binary/kernel/${NFSKERNNAME}
	cd ${.CURDIR}/../distrib/pmax && \
	    ${MAKE} depend && ${MAKE} ${_J} && ${MAKE} install
