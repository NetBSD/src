#	$NetBSD: Makefile,v 1.7.2.2 2000/03/01 00:28:02 he Exp $

TOP=		${.CURDIR}/..

# This include just sets REV=XX
.include "${TOP}/Makefile.inc"

IMAGE=	ramdisk
CBIN=	rd_bin

TREE=	${TOP}/common/${IMAGE}.tree

LISTS=	${TOP}/common/${CBIN}.list \
	${TOP}/common/${IMAGE}.list

KERNEL3  = ${KERNOBJDIR}/RAMDISK/netbsd
KERNEL3X = ${KERNOBJDIR}/RAMDISK3X/netbsd

MOUNT_POINT?=	/mnt
# DEV/RDEV file system device, CDEV/CRDEV vnconfig device
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c

CYLS=		20
SECS=		16
TRKS=		2
TOTALSECT !=	expr ${CYLS} '*' ${SECS} '*' ${TRKS}
DISKTYPE=	rdroot
DISKTAB=	./${DISKTYPE}.disktab
GEOM=		512/${SECS}/${TRKS}/${CYLS}
INO_BYTES=	2048

KERNELS= netbsd-RAMDISK netbsd-RAMDISK3X
CLEANFILES= $(KERNELS) rdsetroot ${IMAGE}.fs ${DISKTAB}

all: $(KERNELS)

netbsd-RAMDISK : rdsetroot ${IMAGE}.fs
	cp ${KERNEL3} netbsd-tmp
	./rdsetroot  netbsd-tmp < ${IMAGE}.fs
	-mv -f netbsd-tmp $@

netbsd-RAMDISK3X : rdsetroot ${IMAGE}.fs
	cp ${KERNEL3X} netbsd-tmp
	./rdsetroot  netbsd-tmp < ${IMAGE}.fs
	-mv -f netbsd-tmp $@

rdsetroot: ${TOP}/common/rdsetroot.c
	$(CC) -o $@ -DDEBUG ${TOP}/common/rdsetroot.c

${DISKTAB}:
	echo "${DISKTYPE}:ty=simulated:se#512:nt#${TRKS}:ns#${SECS}:nc#${CYLS}:pa#${TOTALSECT}:oa#0:ba#4096:fa#512:ta=4.2BSD:pc#${TOTALSECT}:oc#0:" > $@

${IMAGE}.fs: ${TREE} ${LISTS} ${CBIN} ${DISKTAB}
	dd if=/dev/zero of=$@ bs=512 count=${TOTALSECT}
	vnconfig -v -c ${VND_CDEV} $@ ${GEOM}
	disklabel -rw -f ${DISKTAB} ${VND_CDEV} ${DISKTYPE}
	disklabel -W ${VND_CDEV}
	# bigendian, old format, minfree, opt, b/i, cpg, device
	newfs -B be -O -m 0 -o space -i ${INO_BYTES} -c ${CYLS} ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${TREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} \
	  OBJDIR=${.OBJDIR} TARGDIR=${MOUNT_POINT} \
	  sh ${TOP}/common/RunList.sh ${LISTS}
	sync
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}

# Do not delete this if I change my mind and kill make...
# Yes delete the durn thing, in case the make fails because vnd is busy!!
#.PRECIOUS: ${IMAGE}.fs

# Rules for making ${CBIN} ...
.include "${TOP}/common/Make.crunch"

# This is listed in rd_bin.conf but is built here.
${CBIN} : libhack.o

# Use stubs to eliminate some large stuff from libc
HACKSRC=${TOP}/../utils/libhack
.include "${HACKSRC}/Makefile.inc"

clean cleandir distclean:
	-rm -f a.out core *.core *.o *.cro
	-rm -f ${CLEANFILES}

# XXX -- prevent 'make depend' from bombing out
depend:

# Standard rules needed by the above...
.include <bsd.obj.mk>

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
