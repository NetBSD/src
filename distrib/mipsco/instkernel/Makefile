#	$NetBSD: Makefile,v 1.1 2001/02/19 07:45:39 wdk Exp $

.include "../../../Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

DISKBINDIR=	/installation/diskimage
KERNBINDIR=	/binary/kernel

IMAGE=		diskimage
IMAGESIZE=	4096 # 512 byte blocks, update disktab.diskimage if changed
DISKTAB=	${.CURDIR}/disktab.diskimage
DISKTYPE=	miniroot
NEWFSOPTS?=	-c 32 -B be -i 32768	# don't need many inodes!

KERN=		${KERNOBJDIR}/RAMDISK/netbsd
RAMDISK!=	cd ${.CURDIR}/../ramdisk; ${MAKE} echoimage
CLEANFILES=	netbsd netbsd.gz netbsd.ecoff netbsd.ecoff.gz ${IMAGE}.gz

# DEV/RDEV file system device, CDEV/RDEV vnconfig device
VND?=		vnd0
VND_DEV=	/dev/${VND}c
VND_RDEV=	/dev/r${VND}c
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
#VND_GEOM=	512/18/2/$$((${IMAGESIZE} / (18 * 2)))

MOUNT_POINT?=	/mnt

#
# install bootblock, so that we can boot from there
#
BOOTINSTALL= ${DESTDIR}/usr/mdec/installboot ${VND_CDEV} \
	${DESTDIR}/usr/mdec/bootxx_ffs

all:	${IMAGE}.gz

netbsd.gz: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd
	mdsetimage -v netbsd ${RAMDISK}
.ifdef ECOFF_KERNEL
	elf2ecoff netbsd netbsd.ecoff
	rm -f netbsd.ecoff.gz
	gzip -9 netbsd.ecoff
.endif # ECOFF_KERNEL
	rm -f netbsd.gz
	gzip -9 netbsd

${IMAGE}.gz: netbsd.gz ${DESTDIR}/usr/mdec/boot
	dd if=/dev/zero of=${IMAGE} count=${IMAGESIZE}
	vnconfig ${DISKTYPEARG} -v -c ${VND} ${IMAGE} ${VND_GEOM}
.ifdef notyet
	-disklabel ${VND} > /dev/null
	disklabel -rw -f ${DISKTAB} ${VND} ${DISKTYPE}
.endif
	newfs -B be -m 0 -o space ${NEWFSOPTS} ${VND_DEV}
	${BOOTINSTALL}
	mount ${VND_DEV} ${MOUNT_POINT}
	cp -p ${DESTDIR}/usr/mdec/boot ${MOUNT_POINT}
	cp -p netbsd.gz ${MOUNT_POINT}/netbsd
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND}
	rm -f ${IMAGE}.gz
	gzip -9 ${IMAGE}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND}
	-/bin/rm -f ${IMAGE} ${IMAGE}.gz

realinstall:
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -o root -g wheel \
		-m ${NONBINMODE} ${IMAGE}.gz ${RELEASEDIR}${DISKBINDIR}
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -o root -g wheel \
		-m ${BINMODE} netbsd.gz ${RELEASEDIR}${KERNBINDIR}/install.gz
.ifdef ECOFF_KERNEL
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -o root -g wheel \
		-m ${BINMODE} netbsd.ecoff.gz \
		${RELEASEDIR}${KERNBINDIR}/install.ecoff.gz
.endif #ECOFF_KERNEL

clean cleandir distclean:
	rm -f ${CLEANFILES}

# XXX
depend:

.include <bsd.obj.mk>
