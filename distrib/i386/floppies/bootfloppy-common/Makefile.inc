#	$NetBSD: Makefile.inc,v 1.18.2.3 2000/06/03 14:58:49 he Exp $

# TOP is assumed to be defined by Makefile including this one.

COMMONDIR=	${TOP}/bootfloppy-common

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}d
VND_CRDEV=	/dev/r${VND}d
IMAGE?=		xxx.fs
KERN?=		netbsd-xxx
MDEC=		${DESTDIR}/usr/mdec
STRIP?=		strip --strip-unneeded

LISTS=		${COMMONDIR}/list
RAMDISK	!=cd $(.CURDIR)/../ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

DISKTYPE?=	floppy3
DISKSIZE?=	2880

# Some reasonable values for the -i parameter to newfs are:
#
#   6144	1147k, 189 inodes free
#  16384	1159k,  93 inodes free
# on 1.44M:
# 204800	1407k,  27 inodes free
# (with a 4k blocksize, one cannot get fewer than ~32 inodes allocated)

INO_BYTES?=	204800

CLEANFILES+=	netbsd.ram.gz

all:	netbsd.ram.gz
	dd if=/dev/zero of=${IMAGE} count=${DISKSIZE}
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	newfs -B le -m 0 -o space -i ${INO_BYTES} -c 80 ${VND_RDEV} ${DISKTYPE}
	${MDEC}/installboot -v -f ${MDEC}/biosboot.sym ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	@echo "installing new bootblocks"
	-rm -f ${MOUNT_POINT}/boot
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${IMAGE}

netbsd.ram.gz: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd.tmp
	${DESTDIR}/usr/sbin/mdsetimage -v netbsd.tmp ${RAMDISK}
	${STRIP} -K _pcic_isa_alloc_iobase \
	    -K _pcic_isa_alloc_iosize -K _pcic_isa_intr_alloc_mask netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

release:
	-mkdir -p ${RELEASEDIR}/installation/floppy
	cp -p ${IMAGE} ${RELEASEDIR}/installation/floppy
	gzip -c -9 <${IMAGE} >${RELEASEDIR}/installation/floppy/${IMAGE}.gz

clean cleandir distclean:
	/bin/rm -f *.core ${IMAGE} ${CLEANFILES}

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
