#	$NetBSD: Makefile,v 1.14 2000/01/14 13:47:27 pk Exp $

TOP=		${.CURDIR}/..
MINIROOT=	${.CURDIR}/../../miniroot

.include "${TOP}/Makefile.inc"

KERN=		${TOP}/../../sys/arch/sparc/compile/INSTALL/netbsd

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
FD?=		fd0
FD_RDEV=	/dev/r${FD}a
IMAGE?=		boot.fs
MDEC=		${DESTDIR}/usr/mdec

LISTS=		${.CURDIR}/list
RAMDISK!=	cd ${TOP}/ramdisk/; \
		printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" |\
		${MAKE} -s -f-

DISKTYPE=	floppy

# Some reasonable values for the -i parameter to newfs are:
#
#   6144	1147k, 189 inodes free
#  16384	1159k,  93 inodes free
# 204800	1167k,  29 inodes free
INO_BYTES=	204800

CLEANFILES+=	netbsd.ram.gz netbsd.tmp

all:	netbsd.ram.gz
	dd if=/dev/zero of=${IMAGE} bs=1440k count=1
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	disklabel -W ${VND_CDEV}
	newfs -B be -m 0 -o space -i ${INO_BYTES} -c 80 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	TOPDIR=${TOP} CURDIR=${MINIROOT} OBJDIR=${.OBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${MINIROOT}/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	@echo "installing new bootblocks"
	-rm -f ${MOUNT_POINT}/boot
	/usr/mdec/binstall -v ffs ${MOUNT_POINT}
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}

netbsd.ram.gz: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd.tmp
	mdsetimage -v netbsd.tmp ${RAMDISK}
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

clean cleandir distclean:
	/bin/rm -f *.core ${IMAGE} ${CLEANFILES}

depend install:

real-floppy:
	dd if=${IMAGE} of=${FD_RDEV} bs=32k

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
release: $(IMAGE)
	gzip -c -9 < $(.OBJDIR)/boot.fs \
		> $(RELEASEDIR)/installation/bootfs/boot.fs.gz
.endif	# RELEASEDIR check

.include <bsd.obj.mk>
