#	$NetBSD: Makefile,v 1.8 2002/02/09 13:56:51 lukem Exp $

TOP?=		${.CURDIR}/..
ARCHDIR?=	${.CURDIR}/../miniroot

.include "${TOP}/Makefile.inc"

NETBOOTDIR=	${RELEASEDIR}/installation/netboot

IMAGE=		ramdisk.fs
DISKIMAGE=	diskimage.tgz

CBIN=		ramdiskbin
CSIZE=		6144 # 512 byte blocks, update disktab.ramdisk if changed
DISKTAB=	${.CURDIR}/disktab.ramdisk
DISKTYPE=	install-ramdisk
MOUNT_POINT?=	/mnt

VND?=		vnd0
VND_DEV=	/dev/${VND}c
VND_RDEV=	/dev/r${VND}c

CLEANFILES=	${IMAGE} ${DISKIMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache

LISTS=		${.CURDIR}/list
CRUNCHCONF=	${.CURDIR}/${CBIN}.conf
MTREECONF=	${.CURDIR}/mtree.conf

MTREE?=		mtree
CRUNCHGEN?=	crunchgen

realall: ${IMAGE}

${IMAGE}: ${CBIN}
	dd if=/dev/zero of=${IMAGE} count=${CSIZE}
	vnconfig -v -c ${VND} ${IMAGE}
	disklabel -rw -f ${DISKTAB} ${VND} ${DISKTYPE}
	newfs -B be -m 0 -o space -i 5120 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	${MTREE} -def ${MTREECONF} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} ARCHDIR=${ARCHDIR} \
	    KERNOBJDIR=${KERNOBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	@echo Building diskimage tar file...
	tar cf - -C ${MOUNT_POINT} . | gzip -9 > ${DISKIMAGE}
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

.if !defined(RELEASEDIR)
release: .USE
	@echo setenv RELEASEDIR first!
	@false
.else
release: ${DISKIMAGE}
	-mkdir -p ${NETBOOTDIR}
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE} \
		${DISKIMAGE} ${NETBOOTDIR}
.endif # !defined(RELEASEDIR)

realinstall: release

clean cleandir distclean:
	/bin/rm -f *.core ${CLEANFILES} *.o *.cro *.c

.include <bsd.own.mk>
.include <bsd.obj.mk>
