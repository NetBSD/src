#	$NetBSD: Makefile.inc,v 1.4 1997/06/23 21:24:24 thorpej Exp $

# TOP is assumed to be defined by Makefile including this one.

COMMONDIR=	${TOP}/bootfloppy-common

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
IMAGE?=		xxx-${REV}.fs
KERN?=		netbsd-xxx
MDEC=		${DESTDIR}/usr/mdec

LISTS=		${COMMONDIR}/list
RAMDISK	!=cd $(.CURDIR)/../ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk-${REV}.fs\n" | \
	${MAKE} -s -f-

DISKTYPE=	floppy5

CLEANFILES+=	netbsd.ram.gz

all:	netbsd.ram.gz
	dd if=/dev/zero of=${IMAGE} bs=100k count=12
	vnconfig -t ${DISKTYPE} -v -c ${VND_DEV} ${IMAGE}
	newfs -m 0 -o space -i 6144 -c 80 ${VND_RDEV} ${DISKTYPE}
	mount ${VND_DEV} ${MOUNT_POINT}
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	@echo "installing new bootblocks"
	-rm -f ${MOUNT_POINT}/boot
	umount ${MOUNT_POINT}
	/usr/mdec/installboot -v -f /usr/mdec/biosboot.sym ${VND_RDEV}
	vnconfig -u ${VND_DEV}
	cat /*bin/* > /dev/null

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${IMAGE}

netbsd.ram.gz: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd.tmp
	mdsetimage -v netbsd.tmp ${RAMDISK}
	gzip netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

clean cleandir:
	/bin/rm -f *.core ${IMAGE} ${CLEANFILES}

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
