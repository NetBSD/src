#	$NetBSD: Makefile,v 1.1 2002/02/10 01:22:56 gmcgarry Exp $

BINDIR=		/installation/netboot

TOP?=		${.CURDIR}/..
ARCHDIR?=	${.CURDIR}/../miniroot

.include "${TOP}/Makefile.inc"
IMAGE=		ramdisk.fs

CBIN=		ramdiskbin
CSIZE=		2880 # 512 byte blocks, update disktab.ramdisk if changed
DISKTAB=	${.CURDIR}/disktab.ramdisk
DISKTYPE=	install-ramdisk
MOUNT_POINT?=	/mnt

VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a

CLEANFILES=	${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache

LISTS=		${.CURDIR}/list
CRUNCHCONF=	${.CURDIR}/${CBIN}.conf
MTREECONF=	${.CURDIR}/mtree.conf

CRUNCHGEN?=	crunchgen
MTREE?=		mtree

all: ${IMAGE}

${IMAGE}: ${CBIN} 
	dd if=/dev/zero of=${IMAGE} count=${CSIZE}
	vnconfig -v -c ${VND} ${IMAGE}
	disklabel -rw -f ${DISKTAB} ${VND} ${DISKTYPE}
	newfs -B be -m 0 -o space -i 5120 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	${MTREE} -def ${MTREECONF} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} ARCHDIR=${ARCHDIR} \
	    KERNOBJDIR=${KERNOBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/src/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND}
	-/bin/rm -f ${IMAGE}

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	${CRUNCHGEN} -D ${TOP}/../.. -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	${MAKE} -f ${CBIN}.mk all

echoimage:
	@echo ${.OBJDIR}/${IMAGE}

realinstall:
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -o root -g wheel \
		-m ${NONBINMODE} ${DISKIMAGE} ${RELEASEDIR}${BINDIR}

clean cleandir distclean:
	/bin/rm -f *.core ${CLEANFILES} *.o *.cro *.c

# XXX!
depend:
dependall: depend all

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include "../Makefile.inc"
