#	$NetBSD: Makefile,v 1.12.2.3 2000/07/23 16:30:27 he Exp $

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
# DEV/RDEV file system device, CDEV/CRDEV vnconfig device
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c

CYLS=		20
SECS=		64
TRKS=		16
TOTALSECT !=	expr ${CYLS} '*' ${SECS} '*' ${TRKS}
DISKTYPE=	miniroot
DISKTAB=	./${DISKTYPE}.disktab
GEOM=		512/${SECS}/${TRKS}/${CYLS}
INO_BYTES=	8192

CLEANFILES= ${IMAGE}.gz ${IMAGE} install.sub ${DISKTAB}

all: ${IMAGE}.gz

${DISKTAB}:
	echo "${DISKTYPE}:ty=simulated:se#512:nt#${TRKS}:ns#${SECS}:nc#${CYLS}:pa#${TOTALSECT}:oa#0:ba#4096:fa#512:ta=4.2BSD:pc#${TOTALSECT}:oc#0:" > $@

${IMAGE}.gz: ${TREE} ${LISTS} install.sub ${DISKTAB}
	dd if=/dev/zero of=${IMAGE} bs=512 count=${TOTALSECT}
	vnconfig -v -c ${VND_CDEV} ${IMAGE} ${GEOM}
	disklabel -rw -f ${DISKTAB} ${VND_CDEV} ${DISKTYPE}
	disklabel -W ${VND_CDEV}
	# bigendian, old format, minfree, opt, b/i, cpg, device
	newfs -B be -O -m 0 -o space -i ${INO_BYTES} -c ${CYLS} ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${TREE} -p ${MOUNT_POINT}/ -u
	cp ${KERNEL3}  ${MOUNT_POINT}/netbsd.sun3
	cp ${KERNEL3X} ${MOUNT_POINT}/netbsd.sun3x
	TOPDIR=${TOP} CURDIR=${.CURDIR} \
	  OBJDIR=${.OBJDIR} TARGDIR=${MOUNT_POINT} \
	  sh ${TOP}/common/RunList.sh ${LISTS}
	sync ; sleep 1 ; sync
	cd ${MOUNT_POINT} ;\
	  usr/mdec/installboot -v ufsboot usr/mdec/bootxx ${VND_CRDEV}
	sync
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}
	gzip -9 < ${IMAGE} > $@

# Do not delete this if I change my mind and kill make...
.PRECIOUS: ${IMAGE}.gz

install.sub : ${TOP}/../miniroot/install.sub
	sed -e "/^VERSION=/s/=.*/=${REV}/" < $? > $@

clean cleandir distclean:
	-rm -f a.out core *.core *.o
	-rm -f ${CLEANFILES}

# XXX -- allow 'make depend' to run without error
depend:

# Standard rules needed by the above...
.include <bsd.obj.mk>

.if !defined(RELEASEDIR)
release:
	@echo setenv RELEASEDIR before doing that!
	@false
.else	# RELEASEDIR
release: ${IMAGE}.gz
	-mkdir -p ${RELEASEDIR}/installation/miniroot
	cp -p ${IMAGE}.gz \
	 ${RELEASEDIR}/installation/miniroot/${IMAGE}.gz
.endif	# RELEASEDIR
