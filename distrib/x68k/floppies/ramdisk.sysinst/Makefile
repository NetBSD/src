#	$NetBSD: Makefile,v 1.5 2000/06/18 10:33:19 minoura Exp $

TOP=		${.CURDIR}/..
WARNS=1

.include "${TOP}/Makefile.inc"
IMAGE=		ramdisk.fs

AUXDEPENDS= 	dot.profile dot.hdprofile disktab.preinstall \
		termcap.vt

CBIN=		ramdiskbin

MOUNT_POINT?=	/mnt
# DEV/RDEV file system device, CDEV/RDEV vnconfig device
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
IMAGE?=		xxx.fs
MDEC=		${DESTDIR}/usr/mdec

LISTS=		list
CRUNCHCONF=	${CBIN}.conf
MTREE=		mtree.conf

RAMDISKSPT=	36
RAMDISKCYLS=	80
RAMDISKSIZE!=	expr ${RAMDISKSPC} \* ${RAMDISKCYLS} \* 2

realall: ${IMAGE}

${IMAGE}: ${CBIN} ${AUXDEPENDS} ${MTREE} ${LISTS} passwd
	dd if=/dev/zero of=${IMAGE} count=${RAMDISKSIZE}
	vnconfig -v -c ${VND_CDEV} ${IMAGE} 512/${RAMDISKTPC}/2/${RAMDISKCYLS}
	newfs -B be -m 0 -o space -i 5120 -c 80 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${.CURDIR}/${MTREE} -p ${MOUNT_POINT}/ -U
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    KERNOBJDIR=${KERNOBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${.CURDIR}/${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}

passwd:
	-/bin/mkdir ${.OBJDIR}/etc
	/bin/cp -pf ${.CURDIR}/../../../../etc/master.passwd ${.OBJDIR}/etc
	pwd_mkdb -p -d ${.OBJDIR} ${.OBJDIR}/etc/master.passwd
	/bin/cp -pf ${.OBJDIR}/etc/passwd ${.OBJDIR}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${IMAGE}

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	crunchgen -D ${TOP}/../../.. -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	make -f ${CBIN}.mk all

# Use size-reduced libhack.
${CBIN}: libhack.o

HACKSRC=${TOP}/../../utils/libhack
.include "${HACKSRC}/Makefile.inc"

# turn off small gethostby* temporarily
HACKOBJS:= getcap.o getgrent.o getnet.o getnetgr.o getpwent.o localeconv.o perror.o setlocale.o strerror.o strsignal.o utmp.o yplib.o
DBG=	-Os

# Used in building bootfloppy
echoimage:
	@echo ${.OBJDIR}/${IMAGE}

depend:

release:

clean cleandir distclean:
	/bin/rm -f ${AUXCLEAN} *.core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache *.o *.cro *.c

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.sys.mk>
