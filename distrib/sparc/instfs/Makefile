#	$NetBSD: Makefile,v 1.7 2002/05/02 18:02:53 lukem Exp $
#
# instfs.tgz is the filesystem image for disk 2 of the floppy based
# installation method.
# It is constructed by tarring up the contents of the miniroot with
# the exeption of the kernel and boot program.
#

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

MINIROOT=	${.CURDIR}/../../miniroot

MOUNT_POINT?=	/mnt
# DEV/RDEV file system device, CDEV/RDEV vnconfig device
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
INSTFS?=	instfs.tgz

MINIROOTDIR!=	cd ${MINIROOT} && ${PRINTOBJDIR}
MINIROOT_IMAGE=	${MINIROOTDIR}/miniroot.fs

all: ${INSTFS}

${INSTFS}: ${MINIROOT_IMAGE}
	vnconfig -v -c ${VND} ${MINIROOT_IMAGE}
	mount ${VND_DEV} ${MOUNT_POINT}
	(cd ${MOUNT_POINT}; \
		tar -z -X ${.CURDIR}/exclude -c -f ${.OBJDIR}/${.TARGET} .)
	umount ${MOUNT_POINT}
	vnconfig -u ${VND}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND}
	-/bin/rm -f ${INSTFS}

clean cleandir distclean:
	/bin/rm -f *.core ${INSTFS}

release: check_RELEASEDIR .WAIT ${INSTFS}
	${RELEASE_INSTALL} ${INSTFS} $(RELEASEDIR)/installation/bootfs

.include <bsd.prog.mk>
