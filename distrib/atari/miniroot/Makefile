#	$NetBSD: Makefile,v 1.1.1.1 1996/05/19 19:43:39 leo Exp $

# Revision is 1.1B
REV=		11B

CBIN=		instbin
ARCHDIR=	${.CURDIR}/../${MACHINE}/miniroot

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}c
VND_RDEV=	/dev/r${VND}c
IMAGE?=		inst-${REV}.fs
MDEC=		${DESTDIR}/usr/mdec

#LISTS=		${ARCHDIR}/list ${.CURDIR}/list
LISTS=		${.CURDIR}/list ${.CURDIR}/list.md
CRUNCHCONF=	${CBIN}.conf
MTREE=		${.CURDIR}/mtree.conf

all: ${CBIN}
	dd if=/dev/zero of=${IMAGE} bs=18k count=60
	vnconfig -v -c ${VND_DEV} ${IMAGE}
	newfs -m 0 -o space -i 3052 -c 60 ${VND_RDEV} floppybt
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${MTREE} -p ${MOUNT_POINT}/ -u
	CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} ARCHDIR=${ARCHDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${.CURDIR}/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_DEV}
	cat /*bin/* > /dev/null

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${IMAGE}

${CBIN}.conf: ${LISTS}
	awk -f ${.CURDIR}/makeconf.awk CBIN=${CBIN} ${LISTS} > ${CBIN}.conf

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	crunchgen -D ${.CURDIR}/../../.. -L ${DESTDIR}/usr/lib ${CRUNCHCONF}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	make -f ${CBIN}.mk all

clean cleandir:
	/bin/rm -f core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache \
	${CBIN}.conf *.o *.lo *.c

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
