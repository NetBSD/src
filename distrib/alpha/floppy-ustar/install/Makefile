#	$NetBSD: Makefile,v 1.2 1998/09/25 00:33:53 ross Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=		install${REV}.fs
KERN=	${.CURDIR}/../../../../sys/arch/alpha/compile/INSTALL-FLOPPY/netbsd

# TOP is assumed to be defined by Makefile including this one.

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
IMAGE?=		xxx-${REV}.fs
KERN?=		netbsd-xxx
MDEC=		${DESTDIR}/usr/mdec

PRIMARYBOOT=	${DESTDIR}/usr/mdec/bootxx
SECONDARYBOOT=	${DESTDIR}/usr/mdec/boot

LISTS=		${.CURDIR}/list
RAMDISK	!=cd $(.CURDIR)/../md_install/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/md_install-${REV}.fs\n" | \
	${MAKE} -s -f-

DISKTYPE=	floppy

# Some reasonable values for the -i parameter to newfs are:
#
#   6144	1147k, 189 inodes free
#  16384	1159k,  93 inodes free
# 204800	1167k,  29 inodes free
INO_BYTES?=	6144

CLEANFILES+=	netbsd

all:	netbsd
	dd if=/dev/zero of=${IMAGE} count=2880
	@ls -l netbsd | (read mode links uid gid size junk;	\
		bytes=$$((2880 * 512 - $$size - 8192));		\
		echo There are $$bytes \($$(($$bytes / 1024))K\) bytes free.)
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	tar cvf - ${SECONDARYBOOT} netbsd | dd bs=8k seek=1 of=${VND_CDEV}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	@echo "installing new bootblocks"
	${MDEC}/installboot -b 17 -v ${SECONDARYBOOT} ${PRIMARYBOOT} ${VND_CRDEV}
	vnconfig -u ${VND_CDEV}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_CDEV}
	-/bin/rm -f ${IMAGE}

netbsd: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd.tmp
	mdsetimage -v netbsd.tmp ${RAMDISK}
	strip netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

clean cleandir:
	/bin/rm -f *.core ${IMAGE} ${CLEANFILES}

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
