#	$NetBSD: Makefile,v 1.7 2002/03/26 23:09:40 fredette Exp $

TOP=		${.CURDIR}/..

# This include just sets DISTRIBREV=XX
.include "${TOP}/Makefile.inc"
.include <bsd.own.mk>

IMAGE=	ramdisk
CBIN=	rd_bin

TREE=	${TOP}/common/${IMAGE}.tree

LISTS=	${TOP}/common/${CBIN}.list \
	${TOP}/common/${IMAGE}.list

KERNEL  = ${KERNOBJDIR}/RAMDISK/netbsd

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

KERNELS= netbsd.RAMDISK 
CLEANFILES= $(KERNELS) ${IMAGE}.fs netbsd-tmp

all: $(KERNELS)

netbsd.RAMDISK : ${IMAGE}.fs
	cp ${KERNEL} netbsd-tmp
	${MDSETIMAGE} -v netbsd-tmp ${IMAGE}.fs
	-mv -f netbsd-tmp $@

${IMAGE}.fs: ${TREE} ${LISTS} ${CBIN}
	dd if=/dev/zero of=${IMAGE}.fs bs=${SIZE} count=1
	vnconfig -f ${DESTDIR}/etc/disktab -t ${DISKTYPE} -v \
	  -c ${VND} ${IMAGE}.fs
	disklabel -rw -f ${DESTDIR}/etc/disktab ${VND} ${DISKTYPE}
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
	dd if=${VND_DEV} of=${IMAGE}-tmp.fs bs=${SIZE} count=1
	vnconfig -u ${VND}
	mv ${IMAGE}-tmp.fs ${IMAGE}.fs

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND}
	-/bin/rm -f ${IMAGE}.fs

# Do not delete this if I change my mind and kill make...
.PRECIOUS: ${IMAGE}.fs

# Rules for making ${CBIN} ...
.include "${TOP}/common/Make.crunch"

# This is listed in rd_bin.conf but is built here.
${CBIN} : libhack.o

# Use stubs to eliminate some large stuff from libc
HACKSRC=${TOP}/../utils/libhack
CPPFLAGS+=-I${DESTDIR}/usr/include
.include "${HACKSRC}/Makefile.inc"
HACKOBJS+=	gethost.o
libhack.o: gethost.o

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

# Standard rules needed by the above...
.include <bsd.obj.mk>
.include <bsd.sys.mk>
.include <bsd.subdir.mk>
