#	$NetBSD: Makefile,v 1.17 2000/06/10 03:02:14 tsutsui Exp $

TOP=		${.CURDIR}/..

# This include just sets REV=XX
.include "${TOP}/Makefile.inc"

IMAGE=	miniroot

TREE=	${TOP}/common/${IMAGE}.tree

LISTS=	${TOP}/common/${IMAGE}.list \
	${TOP}/common/mini_sbin.list \
	${TOP}/common/mini_bin.list \
	${TOP}/common/mini_usr.list

KERNEL3  = ${KERNOBJDIR}/INSTALL/netbsd
KERNEL3X = ${KERNOBJDIR}/INSTALL3X/netbsd

MOUNT_POINT?=	/mnt
VND?=		vnd1
VND_DEV?=	/dev/${VND}a
VND_RDEV?=	/dev/r${VND}a
VND_CRDEV?=	/dev/r${VND}c

# These are all the parameters for the miniroot: (10MB)
DISKTYPE=	miniroot
SIZE=		10
# bigendian, old format, minfree, opt, b/i, cpg
NEWFSARGS= -B be -O -m 0 -o space -i 8192 -c 16

CLEANFILES= ${IMAGE}.gz ${IMAGE} ${IMAGE}.tmp install.sub

all: ${IMAGE}.gz

${IMAGE}.gz: ${TREE} ${LISTS} install.sub
	dd if=/dev/zero of=${IMAGE} bs=1024k count=${SIZE}
	vnconfig -t ${DISKTYPE} -v -c ${VND} ${IMAGE}
	disklabel -rw ${VND} ${DISKTYPE}
	newfs ${NEWFSARGS} ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${TREE} -p ${MOUNT_POINT}/ -u
	cp ${KERNEL3}  ${MOUNT_POINT}/netbsd.sun3
	cp ${KERNEL3X} ${MOUNT_POINT}/netbsd.sun3x
	TOPDIR=${TOP} CURDIR=${.CURDIR} DESTDIR=${DESTDIR} \
	  OBJDIR=${.OBJDIR} TARGDIR=${MOUNT_POINT} \
	  sh ${TOP}/common/RunList.sh ${LISTS}
	sync ; sleep 1 ; sync
	cd ${MOUNT_POINT} ;\
	  usr/mdec/installboot -v ufsboot usr/mdec/bootxx ${VND_CRDEV}
	sync
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	-umount ${MOUNT_POINT}
	vnconfig -u ${VND}
	gzip -9 -c ${IMAGE} > ${IMAGE}.tmp
	-mv -f ${IMAGE}.tmp ${IMAGE}.gz

# Do not delete this if I change my mind and kill make...
.PRECIOUS: ${IMAGE}.gz

install.sub : ${TOP}/../miniroot/install.sub
	sed -e "/^VERSION=/s/=.*/=${REV}/" < $? > $@

clean cleandir distclean:
	-rm -f a.out core *.core *.o
	-rm -f ${CLEANFILES}

.if !defined(RELEASEDIR)
release:
	@echo setenv RELEASEDIR before doing that!
	@false
.else	# RELEASEDIR
release: ${IMAGE}.gz
	cp -p ${IMAGE}.gz \
	 ${RELEASEDIR}/installation/miniroot/${IMAGE}.gz
.endif	# RELEASEDIR

# Standard rules needed by the above...
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
