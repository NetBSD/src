#	$NetBSD: Makefile.inc,v 1.15.2.1 2000/12/15 05:31:44 he Exp $

# TOP is assumed to be defined by Makefile including this one.

COMMONDIR=	${TOP}/bootfloppy-common

.include <bsd.kernobj.mk>

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
IMAGE?=		xxx-${REV}.fs
RAMDISKDIR?=	$(.CURDIR)/../ramdisk/
KERN?=		netbsd-xxx
KERNNAME?=	netbsd.xxx
GZNAME?=	${KERNNAME}.gz
MDEC=		${DESTDIR}/usr/mdec

LISTS=		${COMMONDIR}/list
.if ${RAMDISKDIR} != ""
RAMDISK	!=cd ${RAMDISKDIR}; ${MAKE} echoimage
.endif

DISKTYPE=	floppy5

# Some reasonable values for the -i parameter to newfs are:
#
#   6144	1147k, 189 inodes free
#  16384	1159k,  93 inodes free
# 204800	1167k,  29 inodes free
INO_BYTES?=	204800

CLEANFILES+=	${KERNNAME} ${GZNAME} netbsd.tmp*

realall: ${GZNAME}
	dd if=/dev/zero of=${IMAGE} bs=100k count=12
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	newfs -B be -m 0 -o space -i ${INO_BYTES} -c 80 ${VND_RDEV} ${DISKTYPE}
	mount ${VND_DEV} ${MOUNT_POINT}
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} KERN=${GZNAME} \
	    KERNOBJDIR=${KERNOBJDIR} \
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

.if defined(RAMDISK)
${KERNNAME}: ${KERN} ${RAMDISK}
	-/bin/rm -f netbsd.tmp*
	cp ${KERN} netbsd.tmp
	mdsetimage -v netbsd.tmp ${RAMDISK}
	mv -f netbsd.tmp ${.TARGET}
.else
${KERNNAME}: ${KERN}
	cp ${KERN} ${.TARGET}
.endif

${GZNAME}: ${KERNNAME}
	gzip -cv9 ${KERNNAME} > ${.TARGET}

depend:

release:
	-mkdir -p ${RELEASEDIR}/installation/floppy
	-mkdir -p ${RELEASEDIR}/installation/misc
	cp -p ${IMAGE} ${RELEASEDIR}/installation/floppy
	gzip <${IMAGE} >${RELEASEDIR}/installation/floppy/${IMAGE}.gz
.ifndef NOKERNCOPY
	cp -p ${KERNNAME} ${RELEASEDIR}/installation/misc
	cp -p ${GZNAME} ${RELEASEDIR}/installation/misc
.endif

clean cleandir distclean:
	/bin/rm -f *.core ${IMAGE} ${CLEANFILES}

.include <bsd.obj.mk>
