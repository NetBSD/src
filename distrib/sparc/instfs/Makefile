#	$NetBSD: Makefile,v 1.6 2002/03/11 07:11:38 lukem Exp $
#
# instfs.tgz is the filesystem image for disk 2 of the floppy based
# installation method.
# It is constructed by tarring up the contents of the miniroot with
# the exeption of the kernel and boot program.
#

MINIROOT=	${.CURDIR}/../../miniroot

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.own.mk>

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

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
release: ${INSTFS}
	cp ${INSTFS} $(RELEASEDIR)/installation/bootfs
.endif	# RELEASEDIR check

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
