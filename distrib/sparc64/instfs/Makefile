#	$NetBSD: Makefile,v 1.1 2001/08/23 15:50:09 mrg Exp $
#
# instfs.tgz is the filesystem image for disk 2 of the floppy based
# installation method.
# It is constructed by tarring up the contents of the miniroot with
# the exeption of the kernel and boot program.
#

TOP=		${.CURDIR}/..
MINIROOT=	${.CURDIR}/../xminiroot

.include "${TOP}/Makefile.inc"

MOUNT_POINT?=	/mnt
# DEV/RDEV file system device, CDEV/RDEV vnconfig device
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
INSTFS?=	instfs.tgz

MINIROOT_IMAGE!=cd ${MINIROOT}; \
		printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/miniroot.fs\n" | \
		${MAKE} -s -f-

all: ${INSTFS}

${INSTFS}: ${MINIROOT_IMAGE}
	vnconfig -v -c ${VND_CDEV} ${MINIROOT_IMAGE}
	mount ${VND_DEV} ${MOUNT_POINT}
	(cd ${MOUNT_POINT}; \
		tar -z -X ${.CURDIR}/exclude -c -f ${.OBJDIR}/${.TARGET} .)
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${INSTFS}

clean cleandir distclean:
	/bin/rm -f *.core ${INSTFS}

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
release: ${INSTFS}
	cp ${INSTFS} $(RELEASEDIR)/installation/misc
.endif	# RELEASEDIR check

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
