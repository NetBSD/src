#	$NetBSD: Makefile,v 1.28 2002/04/26 15:50:49 lukem Exp $


.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

DBG=	-Os

IMAGE=	ramdisk
CBIN=	rd_bin

TREE=	${TOP}/common/${IMAGE}.tree

LISTS=	${TOP}/common/${CBIN}.list \
	${TOP}/common/${IMAGE}.list

KERNEL3  = ${KERNOBJDIR}/RAMDISK/netbsd
KERNEL3X = ${KERNOBJDIR}/RAMDISK3X/netbsd

MOUNT_POINT?=	/mnt
VND?=		vnd1
VND_DEV?=	/dev/${VND}a
VND_RDEV?=	/dev/r${VND}a

# These are all the parameters for the root fs:
DISKTYPE=	rdroot
SIZE=		400k
# bigendian, minfree, opt, b/i , cpg
NEWFSARGS= -B be -m 0 -o space -i 4096 -c 20
MDSETIMAGE?=	mdsetimage
MTREE?=		mtree

KERNELS= netbsd.RAMDISK netbsd.RAMDISK3X
CLEANFILES= $(KERNELS) ${IMAGE}.fs netbsd-tmp

all: $(KERNELS)

netbsd.RAMDISK : ${IMAGE}.fs
	cp ${KERNEL3} netbsd-tmp
	${MDSETIMAGE} -v netbsd-tmp ${IMAGE}.fs
	-mv -f netbsd-tmp $@

netbsd.RAMDISK3X : ${IMAGE}.fs
	cp ${KERNEL3X} netbsd-tmp
	${MDSETIMAGE} -v netbsd-tmp ${IMAGE}.fs
	-mv -f netbsd-tmp $@

${IMAGE}.fs: ${TREE} ${LISTS} ${CBIN}
	dd if=/dev/zero of=${IMAGE}.fs bs=${SIZE} count=1
	vnconfig -t ${DISKTYPE} -v -c ${VND} ${IMAGE}.fs
	disklabel -rw ${VND} ${DISKTYPE}
	newfs ${NEWFSARGS} ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	${MTREE} -def ${TREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} DESTDIR=${DESTDIR} \
	  OBJDIR=${.OBJDIR} TARGDIR=${MOUNT_POINT} \
	  sh ${TOP}/common/RunList.sh ${LISTS}
	sync
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND}
	-/bin/rm -f ${IMAGE}.fs

# Do not delete this if I change my mind and kill make...
.PRECIOUS: ${IMAGE}.fs

# Rules for making ${CBIN} ...
.include "${TOP}/common/Make.crunch"

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CBIN}:	libhack.o

clean cleandir distclean:
	-rm -f a.out core *.core *.o *.cro *.c
	-rm -f ${CLEANFILES}

.if !defined(RELEASEDIR)
release:
	@echo setenv RELEASEDIR before doing that!
	@false
.else	# RELEASEDIR
release: $(KERNELS)
.for x in ${KERNELS}
	gzip -c -9 < ${x} > \
	 ${RELEASEDIR}/binary/kernel/${x}.gz
.endfor # KERNELS
.endif	# RELEASEDIR

.INTERRUPT: unconfig

# Standard rules needed by the above...
.include <bsd.own.mk>
.include <bsd.prog.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
