#	$NetBSD: Makefile,v 1.6 2002/02/09 13:56:51 lukem Exp $

.include "../../../Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

DISKBINDIR=	${RELEASEDIR}/installation/diskimage
KERNBINDIR=	${RELEASEDIR}/binary/kernel
MDSETIMAGE?=	mdsetimage

IMAGE=		diskimage
IMAGESIZE=	4096 # 512 byte blocks, update disktab.diskimage if changed
DISKTAB=	${.CURDIR}/disktab.diskimage
DISKTYPE=	miniroot
NEWFSOPTS?=	-c 32 -B be -i 32768	# don't need many inodes!

MDEC?=		${DESTDIR}/usr/mdec
BOOT_PRIMARY=	${MDEC}/bootxx_ffs
BOOT_SECONDARY=	${MDEC}/boot
INSTALLBOOT?=	${MDEC}/installboot	

KERN=		${KERNOBJDIR}/RAMDISK/netbsd
RAMDISK!=	cd ${.CURDIR}/../ramdisk; ${MAKE} echoimage
CLEANFILES=	netbsd netbsd.gz netbsd.ecoff netbsd.ecoff.gz ${IMAGE}.gz

# DEV/RDEV file system device, CDEV/RDEV vnconfig device
VND?=		vnd0
VND_DEV=	/dev/${VND}c
VND_RDEV=	/dev/r${VND}c
#VND_GEOM=	512/18/2/$$((${IMAGESIZE} / (18 * 2)))

MOUNT_POINT?=	/mnt

realall:	${IMAGE}.gz

netbsd.gz: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd
	${MDSETIMAGE} -v netbsd ${RAMDISK}
.ifdef ECOFF_KERNEL
	elf2ecoff netbsd netbsd.ecoff
	rm -f netbsd.ecoff.gz
	gzip -9 netbsd.ecoff
.endif # ECOFF_KERNEL
	rm -f netbsd.gz
	gzip -9 netbsd

${IMAGE}.gz: netbsd.gz ${BOOT_PRIMARY} ${BOOT_SECONDARY}
	dd if=/dev/zero of=${IMAGE} count=${IMAGESIZE}
	vnconfig ${DISKTYPEARG} -v -c ${VND} ${IMAGE} ${VND_GEOM}
	disklabel -rw -f ${DISKTAB} ${VND} ${DISKTYPE}
	newfs -B be -m 0 -o space ${NEWFSOPTS} ${VND_RDEV}
	${INSTALLBOOT} ${VND_RDEV} ${BOOT_PRIMARY}
	mount ${VND_DEV} ${MOUNT_POINT}
	cp -p ${BOOT_SECONDARY} ${MOUNT_POINT}
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

.if !defined(RELEASEDIR)
release: .USE
	@echo setenv RELEASEDIR first!
	@false
.else
release: ${IMAGE}.gz
	-mkdir -p ${DISKBINDIR} ${KERNBINDIR}
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE} \
		${IMAGE}.gz ${DISKBINDIR}
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE} \
		netbsd.gz ${KERNBINDIR}/install.gz
.if defined(ECOFF_KERNEL)
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE} \
		netbsd.ecoff.gz ${KERNBINDIR}/install.ecoff.gz
.endif # ECOFF_KERNEL
.endif # RELEASEDIR

realinstall: release

clean cleandir distclean:
	rm -f ${CLEANFILES}

.include <bsd.obj.mk>
