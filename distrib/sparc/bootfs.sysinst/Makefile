#	$NetBSD: Makefile,v 1.8 2000/09/04 16:54:39 abs Exp $

TOP=		${.CURDIR}/..
MINIROOT=	${.CURDIR}/../../miniroot

KERN=		${KERNOBJDIR}/INSTALL-SYSINST/netbsd

.include "${TOP}/Makefile.inc"

.include <bsd.own.mk>   # So we use /etc/mk.conf.
.include <bsd.kernobj.mk>

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}${RAW_PART}
VND_CRDEV=	/dev/r${VND}${RAW_PART}
FD?=		fd0
FD_RDEV=	/dev/r${FD}a
IMAGE?=		syboot.fs
MDEC=		${DESTDIR}/usr/mdec

LISTS=		${.CURDIR}/list
RAMDISK!=	cd ${TOP}/ramdisk.sysinst/; \
		printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/syramdisk.fs\n" |\
		${MAKE} -s -f-

DISKTYPE=	floppy

# Some reasonable values for the -i parameter to newfs are:
#
#   6144	1147k, 189 inodes free
#  16384	1159k,  93 inodes free
# 204800	1167k,  29 inodes free
INO_BYTES=	204800

CLEANFILES+=	netbsd.syram.gz netbsd.tmp

${IMAGE}:	netbsd.syram.gz
	dd if=/dev/zero of=${IMAGE} bs=1440k count=1
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	disklabel -W ${VND_CDEV}
	newfs -B be -m 0 -o space -i ${INO_BYTES} -c 80 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	TOPDIR=${TOP} CURDIR=${MINIROOT} OBJDIR=${.OBJDIR} \
	    KERNOBJDIR=${KERNOBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${MINIROOT}/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	@echo "installing new bootblocks"
	-rm -f ${MOUNT_POINT}/boot
	/usr/mdec/binstall -v ffs ${MOUNT_POINT}
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}

all:	${IMAGE}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}

netbsd.syram.gz: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd.tmp
	mdsetimage -v netbsd.tmp ${RAMDISK}
	gzip netbsd.tmp
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
	cp netbsd.syram.gz $(RELEASEDIR)/installation/netboot
	gzip -c -9 < $(.OBJDIR)/$(IMAGE) \
		> $(RELEASEDIR)/installation/bootfs/$(IMAGE).gz
.endif	# RELEASEDIR check

.include <bsd.obj.mk>
