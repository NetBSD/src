#	$NetBSD: Makefile,v 1.16.2.1 1999/12/27 18:28:21 wrstuden Exp $

TOP=		${.CURDIR}/..
WARNS=1

.include "${TOP}/Makefile.inc"
IMAGE=		ramdisk.fs

AUXTARGETS=	install.sh upgrade.sh start.sh
AUXDEPENDS= 	dot.profile dot.hdprofile disktab.preinstall \
		termcap.mini
AUXCLEAN=	${AUXTARGETS}

CBIN=		ramdiskbin

MOUNT_POINT?=	/mnt
# DEV/RDEV file system device, CDEV/RDEV vnconfig device
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}d
VND_CRDEV=	/dev/r${VND}d
IMAGE?=		xxx.fs
MDEC=		${DESTDIR}/usr/mdec

LISTS=		list
CRUNCHCONF=	${CBIN}.conf
MTREE=		mtree.conf

DISKTYPE=	floppy3

install.sh: install.tmpl
	sed "s/@@VERSION@@/${VER}/" < ${.ALLSRC} > ${.TARGET}

upgrade.sh: upgrade.tmpl
	sed "s/@@VERSION@@/${VER}/" < ${.ALLSRC} > ${.TARGET}

start.sh: start.tmpl
	sed "s/@@VERSION@@/${VER}/" < ${.ALLSRC} > ${.TARGET}

all: ${AUXTARGETS} ${CBIN} ${AUXDEPENDS} ${MTREE} ${LISTS}
	dd if=/dev/zero of=${IMAGE} count=2880
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	newfs -B le -m 0 -o space -i 4000 -c 80 ${VND_RDEV} ${DISKTYPE}
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${.CURDIR}/${MTREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${.CURDIR}/${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${IMAGE}

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	crunchgen -D ${TOP}/../../.. -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	make -f ${CBIN}.mk all

# This is listed in ramdiskbin.conf but is built here.
${CBIN}: libhack.o

# Use stubs to eliminate some large stuff from libc
HACKSRC=${TOP}/../../utils/libhack
.include "${HACKSRC}/Makefile.inc"

# turn off small gethostby* temporarily
HACKOBJS:= getcap.o getgrent.o getnet.o getnetgr.o getpwent.o setlocale.o yplib.o

release:

clean cleandir distclean:
	/bin/rm -f ${AUXCLEAN} *.core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache *.o *.cro *.c

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
.include <bsd.sys.mk>
