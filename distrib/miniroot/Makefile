#	$NetBSD: Makefile,v 1.14 1997/07/15 08:16:36 pk Exp $

.include <bsd.own.mk>

REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s

CBIN=		instbin
ARCHDIR=	${.CURDIR}/../${MACHINE}/miniroot

.if exists(${ARCHDIR}/Makefile.inc)
.include "${ARCHDIR}/Makefile.inc"
.endif

MOUNT_POINT?=	/mnt
VND?=		vnd0c
VND_DEV=	/dev/${VND}
VND_RDEV=	/dev/r${VND}
IMAGE?=		miniroot-${REV}.fs
IMAGESIZE?=	12288 # 6Mb in 512 byte blocks
NEWFSOPTS?=

.ifdef DISKTYPE
DISKTYPEARG=	-t ${DISKTYPE}
.endif

LISTS=		${.CURDIR}/list ${ARCHDIR}/list
CRUNCHCONF=	${CBIN}.conf
MTREE=		${.CURDIR}/mtree.conf

.ifndef DESTDIR
all:
	@echo setenv DESTDIR before makeing a miniroot!
	@false
.else

all: ${CBIN}
	dd if=/dev/zero of=${IMAGE} count=${IMAGESIZE}
	vnconfig ${DISKTYPEARG} -v -c ${VND_DEV} ${IMAGE}
	newfs -m 0 -o space ${NEWFSOPTS} ${VND_RDEV} ${DISKTYPE}
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${MTREE} -p ${MOUNT_POINT}/ -u
	REV=${REV} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} ARCHDIR=${ARCHDIR} \
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
	crunchgen -D ${.CURDIR}/../.. -L ${DESTDIR}/usr/lib ${CRUNCHCONF}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	make -f ${CBIN}.mk all

clean cleandir:
	/bin/rm -f core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache \
	${CBIN}.conf *.o *.lo *.c

.endif	# DESTDIR check
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
