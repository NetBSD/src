#	$NetBSD: Makefile,v 1.4 1999/04/16 16:08:28 simonb Exp $


IMAGE=		diskimage
IMAGESIZE=	4096 # 512 byte blocks
DISKTYPE=
LABELPROTO=    ${.CURDIR}/diskimage.label.proto
# Get defaults from minidisk configuration
.include	"../miniroot/Makefile.inc"
NEWFSOPTS+=	-i 32768	# don't need many inodes!

KERN=		${.CURDIR}/../../../sys/arch/pmax/compile/RAMDISK/netbsd
RAMDISK!=	cd ${.CURDIR}/../ramdisk; \
		printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
		${MAKE} -s -f-
CLEANFILES=	netbsd netbsd.gz netbsd.ecoff netbsd.ecoff.gz ${IMAGE}

VND_DEV=	/dev/${VND}
VND_RDEV=	/dev/r${VND}
MOUNT_POINT?=	/mnt

all:	netbsd.gz ${IMAGE}

netbsd.gz: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd
	mdsetimage -v netbsd ${RAMDISK}
	elf2ecoff netbsd netbsd.ecoff
	rm -f netbsd.ecoff.gz
	gzip -9 netbsd.ecoff
	rm -f netbsd.gz
	gzip -9 netbsd

${IMAGE}: netbsd.gz ${DESTDIR}/usr/mdec/boot
	dd if=/dev/zero of=${IMAGE} count=${IMAGESIZE}
	vnconfig ${DISKTYPEARG} -v -c ${VND_DEV} ${IMAGE} ${VND_GEOM}
	${PRELABEL}
	-newfs -m 0 -o space ${NEWFSOPTS} ${VND_RDEV}
	${BOOTINSTALL}
	mount ${VND_DEV} ${MOUNT_POINT}
	cp -p ${DESTDIR}/usr/mdec/boot ${MOUNT_POINT}
	cp -p netbsd.gz ${MOUNT_POINT}/netbsd
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_DEV}

clean cleandir distclean:
	rm -f ${CLEANFILES}

.include <bsd.own.mk>
.include <bsd.obj.mk>
