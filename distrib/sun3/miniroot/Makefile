#	$NetBSD: Makefile,v 1.15 1999/05/21 04:06:20 gwr Exp $

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
BDEV?=		/dev/sd1b
CDEV?=		/dev/rsd1b

# These are all the parameters for the miniroot: (8MB)
DISKTYPE=	miniroot
NBLKS=		20480
# bigendian, old format, minfree, opt, b/i  trks, sects, cpg
NEWFSARGS= -B be -O -m 0 -o space -i 8192 -t 8 -u 32 -c 16

CLEANFILES= ${IMAGE}.gz install.sub

all: ${IMAGE}.gz

${IMAGE}.gz: ${TREE} ${LISTS} install.sub
	-newfs ${NEWFSARGS} -s ${NBLKS} ${CDEV} ${DISKTYPE}
	mount ${BDEV} ${MOUNT_POINT}
	mtree -def ${TREE} -p ${MOUNT_POINT}/ -u
	cp ${KERNEL3}  ${MOUNT_POINT}/netbsd.sun3
	cp ${KERNEL3X} ${MOUNT_POINT}/netbsd.sun3x
	TOPDIR=${TOP} CURDIR=${.CURDIR} \
	  OBJDIR=${.OBJDIR} TARGDIR=${MOUNT_POINT} \
	  sh ${TOP}/common/RunList.sh ${LISTS}
	sync ; sleep 1 ; sync
	cd ${MOUNT_POINT} ;\
	  usr/mdec/installboot -v ufsboot usr/mdec/bootxx ${CDEV}
	sync
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	-umount ${MOUNT_POINT}
	dd if=${CDEV} bs=16b count=`expr ${NBLKS} / 16` |\
	  gzip > ${IMAGE}.tmp
	-mv -f ${IMAGE}.tmp ${IMAGE}.gz

# Do not delete this if I change my mind and kill make...
.PRECIOUS: ${IMAGE}.gz

install.sub : ${TOP}/../miniroot/install.sub
	sed -e "/^VERSION=/s/=.*/=${REV}/" < $? > $@

clean cleandir distclean:
	-rm -f a.out core *.core *.o
	-rm -f ${CLEANFILES}

# Standard rules needed by the above...
.include <bsd.obj.mk>

.if !defined(RELEASEDIR)
release:
	@echo setenv RELEASEDIR before doing that!
	@false
.else	# RELEASEDIR
release: ${IMAGE}.gz
	cp -p ${IMAGE}.gz \
	 ${RELEASEDIR}/installation/miniroot/${IMAGE}.gz
.endif	# RELEASEDIR
