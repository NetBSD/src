#	$NetBSD: Makefile.inc,v 1.4 1999/04/27 04:36:25 simonb Exp $
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

# location of the source diskimage
DISKIMAGE_DIR!=		cd ${.CURDIR}/../distrib/pmax/instkernel && ${MAKE} -s print-objdir
DISKIMAGE=		${DISKIMAGE_DIR}/diskimage.gz

# location of the source netboot diskimage tar file
DISKIMAGE_TAR_DIR!=	cd ${.CURDIR}/../distrib/pmax/ramdisk && ${MAKE} -s print-objdir
DISKIMAGE_TAR=		${DISKIMAGE_TAR_DIR}/diskimage.tgz

# location of the ramdisk install kernel
INSTALLKERN_DIR!=	cd ${.CURDIR}/../distrib/pmax/instkernel && ${MAKE} -s print-objdir
INSTALLKERN=		${INSTALLKERN_DIR}/netbsd.gz
INSTALLKERNNAME=	install.gz

# location and name of the stripped down NFS install kernel
NFSCONF=		INSTALL
NFSKERNNAME=		nfsnetbsd
# We don't know about ${KERNOBJDIR} yet ...
NFSKERNDIR=		${.CURDIR:C/[^\/]+$//}sys/arch/${MACHINE}/compile/${NFSCONF}
NFSKERN=		${NFSKERNDIR}/${NFSKERNNAME}

snap_md_post: ${DISKIMAGE} ${DISKIMAGE_TAR} ${INSTALLKERN}
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -o root -g wheel \
		-m ${BINMODE} ${NFSKERN}.ecoff \
		${RELEASEDIR}/binary/kernel/${NFSKERNNAME}
	gzip -9 ${RELEASEDIR}/binary/kernel/${NFSKERNNAME}
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -o root -g wheel \
		-m ${BINMODE} ${INSTALLKERN} \
		${RELEASEDIR}/binary/kernel/${INSTALLKERNNAME}
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -o root -g wheel \
		-m ${NONBINMODE} ${DISKIMAGE} \
		${RELEASEDIR}/installation/diskimage
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -o root -g wheel \
		-m ${NONBINMODE} ${DISKIMAGE_TAR} \
		${RELEASEDIR}/installation/netboot

${DISKIMAGE} ${DISKIMAGE_TAR} ${INSTALLKERN}:
	cd ${.CURDIR}/../distrib/pmax && ${MAKE} ${_J}
