#	$NetBSD: Makefile,v 1.10 1999/05/18 07:31:56 gwr Exp $

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
BDEV?=		/dev/sd1b
CDEV?=		/dev/rsd1b

# These are all the parameters for the root fs: (320K)
DISKTYPE=	rdroot
NBLKS=		640
# bigendian, old format, minfree, opt, b/i  trks, sects, cpg
NEWFSARGS= -B be -O -m 0 -o space -i 2048 -t 2 -u 16 -c 16

CLEANFILES= netbsd-rd.sun3 netbsd-rd.sun3x rdsetroot ${IMAGE}.fs

all: netbsd-rd.sun3 netbsd-rd.sun3x

netbsd-rd.sun3 : rdsetroot ${IMAGE}.fs
	cp ${KERNEL3} netbsd-tmp
	./rdsetroot  netbsd-tmp < ${IMAGE}.fs
	-mv -f netbsd-tmp $@

netbsd-rd.sun3x : rdsetroot ${IMAGE}.fs
	cp ${KERNEL3X} netbsd-tmp
	./rdsetroot  netbsd-tmp < ${IMAGE}.fs
	-mv -f netbsd-tmp $@

rdsetroot: ${TOP}/common/rdsetroot.c
	$(CC) -o $@ -DDEBUG ${TOP}/common/rdsetroot.c

${IMAGE}.fs: ${TREE} ${LISTS} ${CBIN}
	-newfs ${NEWFSARGS} -s ${NBLKS} ${CDEV} ${DISKTYPE}
	mount ${BDEV} ${MOUNT_POINT}
	mtree -def ${TREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} \
	  OBJDIR=${.OBJDIR} TARGDIR=${MOUNT_POINT} \
	  sh ${TOP}/common/RunList.sh ${LISTS}
	sync
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	-umount ${MOUNT_POINT}
	dd if=${CDEV} of=$@ bs=16b \
	  count=`expr ${NBLKS} / 16`

# Do not delete this if I change my mind and kill make...
.PRECIOUS: ${IMAGE}.fs

# Rules for making ${CBIN} ...
.include "${TOP}/common/Make.crunch"

# This is listed in rd_bin.conf but is built here.
${CBIN} : libhack.o

# Use stubs to eliminate some large stuff from libc
HACKSRC=${TOP}/../utils/libhack
.include "${HACKSRC}/Makefile.inc"

clean cleandir distclean:
	-rm -f a.out core *.core *.o
	-rm -f ${CLEANFILES}

# Standard rules needed by the above...
.include <bsd.obj.mk>
