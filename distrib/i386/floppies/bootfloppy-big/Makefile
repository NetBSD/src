#	$NetBSD: Makefile,v 1.7.4.3 2000/10/18 03:34:43 tv Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

COMMONDIR=	${TOP}/bootfloppy-common
IMAGE=	boot-big.fs
KERN!=	cd $(.CURDIR)/../kernel-ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL.gz\n" | \
	${MAKE} -s -f-

# This Makefile builds a boot image on a 2.88M-sized image.
#
# Since NetBSD currently can't write 2.88M diskettes, it's only
# use is as a boot image for El Torito bootable CD-ROM images.
#
# To prepare a CD, do the following steps:
# 1) create a release(7) tree called NetBSD-1.3I
# 2) create NetBSD-1.3I/boot.i386/ and place boot.fs in that directory
# 3) do ``mkisofs -A "NetBSD ${version}" -b boot.i386/boot.fs -f \
#	-o i386-cd.img -r -T NetBSD-1.3I''
#    (you may omit '-f' if you're not using symlinks in the release tree)
# This should place the image in i386-cd.img, which can hopefully
# be used to burn a CD.

DISKTYPE=	floppy288
DISKSIZE=	5760
METAFILE!=	printf "USTAR.volsize.%o" ${DISKSIZE}
BLOCK8K=	359
PAD=yes

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}d
VND_CRDEV=	/dev/r${VND}d
MDEC=		${DESTDIR}/usr/mdec
BOOTCODE=	${MDEC}/biosboot.sym
STRIP?=		strip

LISTS?=		${COMMONDIR}/list

# Some reasonable values for the -i parameter to newfs are:
#
#   6144	1147k, 189 inodes free
#  16384	1159k,  93 inodes free
# on 1.44M:
# 204800	1407k,  27 inodes free
# (with a 4k blocksize, one cannot get fewer than ~32 inodes allocated)

INO_BYTES?=	204800

CLEANFILES+=	netbsd

realall:	netbsd
	-rm -f ${IMAGE}.tmp
	dd if=/dev/zero of=${IMAGE}.tmp count=${DISKSIZE}
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}.tmp
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	newfs -B le -m 0 -o space -i ${INO_BYTES} -c 80 ${VND_RDEV} ${DISKTYPE}
	${MDEC}/installboot -v -f ${BOOTCODE} ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}
	mv -f ${IMAGE}.tmp ${IMAGE}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${IMAGE} ${IMAGE}.tmp

netbsd: ${KERN}
	rm -f netbsd
	cp ${KERN} netbsd

release:
	-mkdir -p ${RELEASEDIR}/installation/floppy
	cp -p ${IMAGE} ${RELEASEDIR}/installation/floppy
	gzip <${IMAGE} >${RELEASEDIR}/installation/floppy/${IMAGE}.gz

clean cleandir distclean:
	/bin/rm -f *.core ${CLEANFILES} ${IMAGE} ${IMAGE}.tmp

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
