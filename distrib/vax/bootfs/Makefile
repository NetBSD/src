#	$NetBSD: Makefile,v 1.1.2.2 2000/11/04 19:43:39 he Exp $

# TOP is assumed to be defined by Makefile including this one.

TOP=	${.CURDIR}/..

.include <bsd.own.mk>

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
IMAGE?=		boot.fs
KERN?=		${KERNOBJDIR}/INSTALL/netbsd
MDEC=		${DESTDIR}/usr/mdec
STRIP?=		strip

BOOTROOTIMAGE=	bootroot-${REV}.fs
BOOTROOTTYPE=	bootroot
BOOTROOTSIZE=	208

LISTS=		${.CURDIR}/list
RAMDISK	!=cd $(.CURDIR)/../ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

DISKTYPE?=	miniroot
DISKSIZE?=	2000

# Some reasonable values for the -i parameter to newfs are:
#
#   6144	1147k, 189 inodes free
#  16384	1159k,  93 inodes free
# 204800	1167k,  29 inodes free
INO_BYTES?=	204800

CLEANFILES+=	netbsd.ram.gz

all:	unconfig bootroot boot.fs

boot.fs: netbsd.ram.gz
	dd if=/dev/zero of=${IMAGE} count=${DISKSIZE}
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	newfs -m 0 -o space -i ${INO_BYTES} -c 2 ${VND_RDEV} ${DISKTYPE}
	mount ${VND_DEV} ${MOUNT_POINT}
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	@echo "installing new bootblocks"
	-disklabel ${VND} > blks
	disklabel -R -r ${VND} blks
	rm -f blks
	disklabel -B -b ${MDEC}/xxboot ${VND}
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_CDEV}
	-/bin/rm -f ${IMAGE}

netbsd.ram.gz: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd.tmp
	mdsetimage -v netbsd.tmp ${RAMDISK}
	${STRIP} netbsd.tmp
	gzip netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

bootroot:
	dd if=/dev/zero of=${BOOTROOTIMAGE} count=${BOOTROOTSIZE}
	vnconfig -t ${BOOTROOTTYPE} -v -c ${VND_CDEV} ${BOOTROOTIMAGE}
	newfs -B le -O -m 0 -o space -c 2 -i 8192 ${VND_RDEV} ${BOOTROOTTYPE}
	mount ${VND_DEV} ${MOUNT_POINT}
	cp ${DESTDIR}/boot ${MOUNT_POINT}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	@echo "installing new bootblocks"
	-disklabel ${VND} > blks
	disklabel -R -r ${VND} blks
	rm -f blks
	disklabel -B -b ${MDEC}/xxboot ${VND}
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
release: $(IMAGE)
	gzip -c -9 < $(.OBJDIR)/${IMAGE} \
		> $(RELEASEDIR)/installation/bootfs/${IMAGE}.gz
	gzip -c -9 < $(.OBJDIR)/${BOOTROOTIMAGE} \
		> $(RELEASEDIR)/installation/bootfs/${BOOTROOTIMAGE}.gz
	cp $(.OBJDIR)/netbsd.ram.gz $(RELEASEDIR)/installation/netboot/.
.endif	# RELEASEDIR check

clean cleandir distclean:
	/bin/rm -f *.core ${IMAGE} ${CLEANFILES}

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
