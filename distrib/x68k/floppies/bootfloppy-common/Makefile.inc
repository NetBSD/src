#	$NetBSD: Makefile.inc,v 1.8.2.3 2000/07/27 17:55:22 he Exp $

# TOP is assumed to be defined by Makefile including this one.

COMMONDIR=	${TOP}/bootfloppy-common

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
IMAGE?=		xxx-${REV}.fs
KERN?=		netbsd-xxx
KERNNAME?=	netbsd.xxx
GZNAME?=	${KERNNAME}.gz
MDEC=		${DESTDIR}/usr/mdec

LISTS=		${COMMONDIR}/list
RAMDISK	!=cd $(.CURDIR)/../ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk-${REV}.fs\n" | \
	${MAKE} -s -f-

DISKTYPE=	floppy5

# Some reasonable values for the -i parameter to newfs are:
#
#   6144	1147k, 189 inodes free
#  16384	1159k,  93 inodes free
# 204800	1167k,  29 inodes free
INO_BYTES?=	204800

CLEANFILES+=	${KERNNAME} ${GZNAME} netbsd.tmp*

all:	${GZNAME}
	dd if=/dev/zero of=${IMAGE} bs=100k count=12
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	newfs -B be -m 0 -o space -i ${INO_BYTES} -c 80 ${VND_RDEV} ${DISKTYPE}
	mount ${VND_DEV} ${MOUNT_POINT}
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} KERN=${GZNAME} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	umount ${MOUNT_POINT}
	@echo ""
	@echo "installing new bootblocks"
	${MDEC}/installboot ${MDEC}/fdboot ${VND_CRDEV}
	vnconfig -u ${VND_CDEV}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${IMAGE}

${KERNNAME}: ${KERN} ${RAMDISK}
	-/bin/rm -f netbsd.tmp*
	cp ${KERN} netbsd.tmp
	mdsetimage -v netbsd.tmp ${RAMDISK}
	mv -f netbsd.tmp ${.TARGET}

${GZNAME}: ${KERNNAME}
	gzip -cv9 ${KERNNAME} > ${.TARGET}

release:
	cp -p ${IMAGE} ${RELEASEDIR}/installation/floppy
	gzip -cv9 ${IMAGE} > ${RELEASEDIR}/installation/floppy/${IMAGE}.gz
	cp -p ${GZNAME} ${RELEASEDIR}/installation/misc
	cp -p ${KERNNAME} ${RELEASEDIR}/installation/misc
	cp -p ${MDEC}/loadbsd.x ${RELEASEDIR}/installation/misc

clean cleandir distclean:
	/bin/rm -f *.core ${IMAGE} ${CLEANFILES}

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
