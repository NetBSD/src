#
#	$NetBSD: Makefile.inc,v 1.2 1997/12/12 22:39:01 gwr Exp $
#

# TOP is assumed to be defined by Makefile including this one.

CBIN?=		instbin
COMMONDIR=	${TOP}/common

MOUNT_POINT?=	/mnt
BDEV?=		/dev/md0
DISKTYPE?=	xxx-disktype
IMAGE?=		xxx-${REV}.fs
MDEC=		${DESTDIR}/usr/mdec

LISTS=		${.CURDIR}/list
MTREE=		${.CURDIR}/mtree.conf

fsimage:
	newfs -O -m 0 -o space -i 3052 -c 60 ${BDEV} ${DISKTYPE}
	mount ${BDEV} ${MOUNT_POINT}
	mtree -def ${MTREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/Runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	-umount ${MOUNT_POINT}

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
