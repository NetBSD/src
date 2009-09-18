#	$NetBSD: Makefile.inc,v 1.16 2009/09/18 09:25:00 abs Exp $

# TOP is assumed to be defined by Makefile including this one.

CBIN=		instbin
COMMONDIR=	${TOP}/inst-common

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}c
VND_RDEV=	/dev/r${VND}c
IMAGE?=		miniroot-${DISTRIBREV}.fs
BOOTSYS=	bootroot-${DISTRIBREV}.fs
MDEC=		${DESTDIR}/usr/mdec

LISTS=		${COMMONDIR}/list ${.CURDIR}/list
CRUNCHCONF=	${COMMONDIR}/${CBIN}.conf
MTREECONF=	${COMMONDIR}/mtree.conf

DISKTYPE=	miniroot
BOOTDISKTYPE=	bootroot

all: ${CBIN} bootroot
	dd if=/dev/zero of=${IMAGE} bs=64k count=32
	vnconfig -t ${DISKTYPE} -v -c ${VND_DEV} ${IMAGE}
	newfs -B le -O -m 0 -o space -i 8192 ${VND_RDEV} ${DISKTYPE}
	mount ${VND_DEV} ${MOUNT_POINT}
	${TOOL_MTREE} -def ${MTREECONF} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    KERNOBJDIR=${KERNOBJDIR} TARGDIR=${MOUNT_POINT} ${HOST_SH} \
	    ${NETBSDSRCDIR}/distrib/common/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount -f ${MOUNT_POINT}
	vnconfig -u ${VND_DEV}

bootroot: edlabel copy xxboot boot
	dd if=/dev/zero of=${BOOTSYS} bs=64k count=4
	vnconfig -t ${BOOTDISKTYPE} -v -c ${VND_DEV} ${BOOTSYS}
	newfs -B le -O -m 0 -o space -i 8192 ${VND_RDEV} ${BOOTDISKTYPE}
	dd if=xxboot of=/dev/rvnd0a count=16
	mount ${VND_DEV} ${MOUNT_POINT}
	cp edlabel copy boot ${MOUNT_POINT}
	umount -f ${MOUNT_POINT}
	vnconfig -u ${VND_DEV}

edlabel copy xxboot boot:
	(cd /sys/arch/vax/boot; ${MAKE})
	cp /sys/arch/vax/boot/edlabel .
	cp /sys/arch/vax/boot/copy .
	cp /sys/arch/vax/boot/xxboot .
	cp /sys/arch/vax/boot/boot .

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV} ${IMAGE}
	-/bin/rm -f ${IMAGE}

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	${TOOL_CRUNCHGEN} ${CRUNCHCONF}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	${MAKE} -f ${CBIN}.mk

clean cleandir distclean:
	/bin/rm -f core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache *.o *.cro *.c

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
