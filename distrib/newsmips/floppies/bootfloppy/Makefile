#	$NetBSD: Makefile,v 1.6 2002/03/05 21:23:30 thorpej Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

IMAGE=		boot.fs
KERN=		${KERNOBJDIR}/INSTALL/netbsd

MOUNT_POINT?=	/mnt
MDSETIMAGE?=	mdsetimage
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
FD?=		fd0
FD_RDEV=	/dev/r${FD}a
MDEC=		${DESTDIR}/usr/mdec

LISTS=		${.CURDIR}/list

RAMDISKDIR!=	cd $(.CURDIR)/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

DISKTYPE=	floppy

# Some reasonable values for the -i parameter to newfs are:
#
#  8192		1387k, 189 inodes free
# 16384		1399k,  93 inodes free
# 65536		1407k,  29 inodes free
INO_BYTES?=	65536

CLEANFILES+=	netbsd.ram.gz netbsd.tmp

all:	netbsd.ram.gz
	dd if=/dev/zero of=${IMAGE} bs=1440k count=1
	# vnconfig -v -c ${VND} ${IMAGE}
	vnconfig -t ${DISKTYPE} -v -c ${VND} ${IMAGE}
	disklabel -f ${DESTDIR}/etc/disktab -rw ${VND} ${DISKTYPE}
	disklabel -W ${VND}
	newfs -B be -m 0 -o space -i ${INO_BYTES} -c 80 \
	    -f 1024 -b 8192 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    KERNOBJDIR=${KERNOBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	@echo "installing new bootblocks"
	-rm -f ${MOUNT_POINT}/boot
	cp ${MDEC}/boot ${MOUNT_POINT}/boot
	sync
	${MDEC}/installboot -v ${MOUNT_POINT}/boot ${MDEC}/bootxx ${VND_RDEV}
	umount ${MOUNT_POINT}
	vnconfig -u ${VND}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND}

netbsd.ram.gz: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd.tmp
	${MDSETIMAGE} -v netbsd.tmp ${RAMDISK}
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

.ifndef RELEASEDIR
release:
	@echo set RELEASEDIR first!
	@false
.else
release:
	-mkdir -p ${RELEASEDIR}/installation/floppy
	cp -p ${IMAGE} ${RELEASEDIR}/installation/floppy
	gzip -c -9 ${IMAGE} > ${RELEASEDIR}/installation/floppy/${IMAGE}.gz
.endif

clean cleandir distclean:
	rm -f *.core ${IMAGE} ${CLEANFILES}

real-floppy:
	dd if=${IMAGE} of=${FD_RDEV} bs=45k

.include <bsd.obj.mk>
