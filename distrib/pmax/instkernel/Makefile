#	$NetBSD: Makefile,v 1.14 2000/01/04 01:20:23 simonb Exp $

DISKBINDIR=	/installation/diskimage
KERNBINDIR=	/binary/kernel

IMAGE=		diskimage
IMAGESIZE=	4096 # 512 byte blocks
DISKTYPE=
LABELPROTO=    ${.CURDIR}/diskimage.label.proto
# Get defaults from minidisk configuration
.include	"../miniroot/Makefile.inc"
NEWFSOPTS+=	-i 32768	# don't need many inodes!

KERN=		${.CURDIR}/../../../sys/arch/pmax/compile/RAMDISK/netbsd
RAMDISK!=	cd ${.CURDIR}/../ramdisk; ${MAKE} echoimage
CLEANFILES=	netbsd netbsd.gz netbsd.ecoff netbsd.ecoff.gz ${IMAGE}.gz

VND_DEV=	/dev/${VND}
VND_RDEV=	/dev/r${VND}
MOUNT_POINT?=	/mnt

all:	netbsd.gz ${IMAGE}.gz

netbsd.gz: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd
	mdsetimage -v netbsd ${RAMDISK}
	elf2ecoff netbsd netbsd.ecoff
	rm -f netbsd.ecoff.gz
	gzip -9 netbsd.ecoff
	rm -f netbsd.gz
	gzip -9 netbsd

${IMAGE}.gz: netbsd.gz ${DESTDIR}/usr/mdec/boot.pmax
	dd if=/dev/zero of=${IMAGE} count=${IMAGESIZE}
	vnconfig ${DISKTYPEARG} -v -c ${VND_DEV} ${IMAGE} ${VND_GEOM}
	${PRELABEL}
	-newfs -m 0 -o space ${NEWFSOPTS} ${VND_RDEV}
	${BOOTINSTALL}
	mount ${VND_DEV} ${MOUNT_POINT}
	cp -p ${DESTDIR}/usr/mdec/boot.pmax ${MOUNT_POINT}
	cp -p netbsd.gz ${MOUNT_POINT}/netbsd
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_DEV}
	rm -f ${IMAGE}.gz
	gzip -9 ${IMAGE}

realinstall:
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -o root -g wheel \
		-m ${NONBINMODE} ${IMAGE}.gz ${RELEASEDIR}${DISKBINDIR}
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -o root -g wheel \
		-m ${BINMODE} netbsd.gz ${RELEASEDIR}${KERNBINDIR}/install.gz
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -o root -g wheel \
		-m ${BINMODE} netbsd.ecoff.gz \
		${RELEASEDIR}${KERNBINDIR}/install.ecoff.gz

clean cleandir distclean:
	rm -f ${CLEANFILES}

# XXX!
depend:

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include "../Makefile.inc"
