#	$NetBSD: Makefile,v 1.4.2.2 1999/11/09 15:48:03 mycroft Exp $

TOP=		${.CURDIR}/..
MINIROOT=	${TOP}/../miniroot
HACKSRC=	${TOP}/../utils/libhack
SRCROOT=	${TOP}/../..
WARNS=1

.if ${MACHINE_ARCH} == "i386"
RAW_PART=	d
.else
RAW_PART=	c
.endif

# Need to add more cases
.if ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "m68k"
FS_END=		be
.else		# i386, pmax
FS_END=		le
.endif

AUXDEPENDS= 	dot.profile disktab.preinstall termcap.mini \
		dist/base_obsolete dist/comp_obsolete \
		dist/games_obsolete dist/man_obsolete dist/misc_obsolete \
		dist/secr_obsolete dist/xbase_obsolete dist/xserver_obsolete
AUXCLEAN=	list ${CBIN}.conf dot.profile

CBIN=		ramdiskbin

MOUNT_POINT?=	/mnt
# DEV/RDEV file system device, CDEV/RDEV vnconfig device
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}${RAW_PART}
VND_CRDEV=	/dev/r${VND}${RAW_PART}
IMAGE?=		syramdisk-${REV}.fs
CSIZE?=         3312
MDEC=		${DESTDIR}/usr/mdec

LISTS=		list
CRUNCHCONF=	${CBIN}.conf
MTREE=		mtree.conf

DISKTYPE?=	floppyinstfs

all: ${CBIN} ${AUXDEPENDS} ${MTREE} ${LISTS} dot.profile
	dd if=/dev/zero of=${IMAGE} count=${CSIZE}
	vnconfig -v -c ${VND_CDEV} ${IMAGE}
	disklabel -f ${.CURDIR}/disktab -rw ${VND_CDEV} ${DISKTYPE}
	newfs -B ${FS_END} -m 0 -o space -i 5120 -c 80 ${VND_RDEV} ${DISKTYPE}
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${.CURDIR}/${MTREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${IMAGE}

dot.profile: dot.profile.m4
	m4 -DMACHINE_ARCH=${MACHINE_ARCH} -DMACHINE=${MACHINE} \
				${.ALLSRC} > ${.TARGET}

${LISTS}: ${LISTS}.m4
	m4 -DMACHINE_ARCH=${MACHINE_ARCH} -DMACHINE=${MACHINE} \
			-DSRCROOT=${SRCROOT} ${.ALLSRC} > ${.TARGET}

${CBIN}.conf: ${CBIN}.m4
	m4 -DMACHINE_ARCH=${MACHINE_ARCH} -DMACHINE=${MACHINE} \
				${.ALLSRC} > ${.TARGET}

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	crunchgen -D ${SRCROOT} -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	make -f ${CBIN}.mk all

# This is listed in ramdiskbin.conf but is built here.
${CBIN}: libhack.o

# Use stubs to eliminate some large stuff from libc
.include "${HACKSRC}/Makefile.inc"

# turn off small gethostby* temporarily
HACKOBJS:= getcap.o getgrent.o getnet.o getnetgr.o getpwent.o setlocale.o yplib.o

clean cleandir distclean:
	/bin/rm -f *.core ${AUXCLEAN} ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache *.o *.lo *.c

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
.include <bsd.sys.mk>
