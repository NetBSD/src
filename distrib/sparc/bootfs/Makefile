#	$NetBSD: Makefile,v 1.34 2002/05/02 18:02:53 lukem Exp $
#
# boot.fs is the image for disk 1 of the two-set floppy based installation
# method.
#
# It is constructed by injecting the microroot filesystem `ramdisk.fs'
# into the md based kernel built from the INSTALL kernel configuration file.
#

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

TOP=		${.CURDIR}/..
MINIROOT=	${.CURDIR}/../../miniroot


KERN?=		${KERNOBJDIR}/INSTALL/netbsd

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
MDSETIMAGE?=	mdsetimage
OBJCOPY?=	objcopy

LISTS=		${.CURDIR}/list

RAMDISKDIR!=	cd ${TOP}/ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

# Some reasonable values for the -i parameter to newfs are:
#
#   6144	1147k, 189 inodes free
#  16384	1159k,  93 inodes free
# 204800	1167k,  29 inodes free
INO_BYTES=	204800

BLOCKSIZE=512
FSSIZE!= expr ${BLOCKSIZE} \* 18 \* 2 \* 80

GEOM=${BLOCKSIZE}/18/2/80

CLEANFILES+=	netbsd.ram netbsd.ram.aout netbsd.tmp

.MAIN: all

all:	netbsd.ram netbsd.ram.aout
	dd if=/dev/zero of=${IMAGE} bs=${FSSIZE} count=1
	vnconfig -v -c ${VND} ${IMAGE} ${GEOM}
	newfs -B be -m 0 -o space -i ${INO_BYTES} -c 80 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	gzip -9 < netbsd.ram > ${MOUNT_POINT}/netbsd
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	@echo "installing new bootblocks"
	-rm -f ${MOUNT_POINT}/boot
	${MDEC}/binstall -m ${MDEC} -f ${IMAGE} -v ffs ${MOUNT_POINT}
	umount ${MOUNT_POINT}
	vnconfig -u ${VND}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND}

netbsd.ram: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd.tmp
	${MDSETIMAGE} -v netbsd.tmp ${RAMDISK}
	strip netbsd.tmp
	mv netbsd.tmp ${.TARGET}


# conjure up a magic header that is accepted by all Sun PROMS;
# see sys/arch/sparc/stand/installboot/installboot.c for details.
SUN_MAGIC_HEADER='\01\03\01\07\060\200\0\07\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0'

netbsd.ram.aout: netbsd.ram
	${OBJCOPY} -O binary ${.ALLSRC} netbsd.ram.raw
	(printf ${SUN_MAGIC_HEADER}; cat netbsd.ram.raw)  > ${.TARGET}
	rm -f netbsd.ram.raw

clean cleandir distclean:
	/bin/rm -f *.core ${IMAGE} ${CLEANFILES}

depend install:

real-floppy:
	dd if=${IMAGE} of=${FD_RDEV} bs=32k

release: check_RELEASEDIR .WAIT boot.fs netbsd.ram.aout
	gzip -c -9 boot.fs > $(RELEASEDIR)/installation/bootfs/boot.fs.gz
	gzip -c -9 netbsd.ram.aout > \
	    $(RELEASEDIR)/installation/bootfs/netbsd.ram.aout.gz

.include <bsd.prog.mk>
