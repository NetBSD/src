#	$NetBSD: Makefile,v 1.4 2000/12/17 09:57:49 jmc Exp $

TOP=		${.CURDIR}/..
WARNS=1

.include "${TOP}/Makefile.inc"
IMAGE=		ramdisk-small.fs

AUXDEPENDS= 	dot.profile disktab.preinstall  termcap.mini

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

CRUNCHGEN?=	crunchgen

RAMDISKSPC=	165
RAMDISKCYLS=	16
RAMDISKSIZE!=	expr ${RAMDISKSPC} \* ${RAMDISKCYLS}

realall: ${IMAGE}

${IMAGE}: ${CBIN} ${AUXDEPENDS} ${MTREE} ${LISTS}
	rm -f ${IMAGE}.tmp
	dd if=/dev/zero of=${IMAGE}.tmp count=${RAMDISKSIZE}
	vnconfig -v -c ${VND_CDEV} ${IMAGE}.tmp \
	    512/${RAMDISKSPC}/1/${RAMDISKCYLS}
	-disklabel ${VND_CDEV} >tmplabel
	disklabel -r -R ${VND_CDEV} tmplabel
	newfs -B le -m 0 -o space -i 3950 -c 80 ${VND_RDEV} 
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${.CURDIR}/${MTREE} -p ${MOUNT_POINT}/ -U
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${.CURDIR}/${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}
	mv -f ${IMAGE}.tmp ${IMAGE}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${IMAGE} ${IMAGE}.tmp

${CBIN}.mk ${CBIN}.cache ${CBIN}.c! ${CRUNCHCONF}
	${CRUNCHGEN} -D ${TOP}/../../.. -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	${MAKE} -f ${CBIN}.mk all

# This is listed in ramdiskbin.conf but is built here.
${CBIN}: libhack.o

# Use stubs to eliminate some large stuff from libc
HACKSRC=${TOP}/../../utils/libhack
.include "${HACKSRC}/Makefile.inc"

# turn off small gethostby* temporarily
HACKOBJS:= getcap.o getgrent.o getnet.o getnetgr.o getpwent.o setlocale.o yplib.o

release:

clean cleandir distclean:
	/bin/rm -f *.core ${IMAGE} ${IMAGE}.tmp ${CBIN} \
	    ${CBIN}.mk ${CBIN}.cache *.o *.cro *.c tmplabel

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
.include <bsd.sys.mk>
