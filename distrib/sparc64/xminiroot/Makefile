#	$NetBSD: Makefile,v 1.6 2001/10/10 05:13:55 enami Exp $

TOP=		${.CURDIR}/..
MINIROOT=	${TOP}/../miniroot

.include "${TOP}/Makefile.inc"

.include <bsd.own.mk>	# So we use /etc/mk.conf.
.include <bsd.kernobj.mk>

HACKSRC=	${TOP}/../utils/libhack
SRCROOT=	${TOP}/../..
WARNS=1
KERN?=		${KERNOBJDIR}/INSTALL/netbsd

.if ${MACHINE_ARCH} == "i386"
RAW_PART=	d
.else
RAW_PART=	c
.endif

# Need to add more cases
.if ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "m68k" || \
    ${MACHINE_ARCH} == "sparc64"
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
VND_DEV=	/dev/${VND}c
VND_RDEV=	/dev/r${VND}c
VND_CDEV=	/dev/${VND}${RAW_PART}
VND_CRDEV=	/dev/r${VND}${RAW_PART}
CSIZE?=		14000
IMAGE?=		miniroot.fs
MDEC=		${DESTDIR}/usr/mdec

LISTS=		list
CRUNCHCONF=	${CBIN}.conf
MTREECONF=	mtree.conf

CRUNCHGEN?=	crunchgen
DISKTYPE?=	miniroot

all: netbsd.INSTALL ${IMAGE}

netbsd.INSTALL: ${KERN} ${IMAGE}
	cp ${KERN} netbsd.tmp
	mdsetimage -v netbsd.tmp ${IMAGE}
	strip netbsd.tmp
	mv netbsd.tmp ${.TARGET}


${IMAGE}: ${CBIN} ${AUXDEPENDS} ${MTREECONF} ${LISTS} dot.profile
	dd if=/dev/zero of=${IMAGE} count=${CSIZE}
	vnconfig -v -c ${VND_CDEV} ${IMAGE}
	-disklabel -f ${.CURDIR}/disktab -rw ${VND_CDEV} ${DISKTYPE}
	#newfs -B ${FS_END} -m 0 -o space -i 5120 -c 80 ${VND_RDEV} ${DISKTYPE}
	newfs -B ${FS_END} -m 0 -o space ${VND_RDEV} ${DISKTYPE}
	mount -o softdep ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${.CURDIR}/${MTREECONF} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    KERNOBJDIR=${KERNOBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	@echo "installing new bootblocks"
	#${MDEC}/binstall -m ${DESTDIR}/usr/mdec -u -v ffs ${MOUNT_POINT}
	umount ${MOUNT_POINT}
	dd if=${MDEC}/bootblk of=${VND_CDEV} bs=512 count=15 conv=notrunc seek=1
	#${MDEC}/installboot -v ${MDEC}/bootblk ${VND_CDEV}
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
	${CRUNCHGEN} -D ${SRCROOT} -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	${MAKE} -f ${CBIN}.mk all

# This is listed in ramdiskbin.conf but is built here.
${CBIN}: libhack.o

# Use stubs to eliminate some large stuff from libc
.include "${HACKSRC}/Makefile.inc"

# turn off small gethostby* temporarily
HACKOBJS:= getcap.o getgrent.o getnet.o getnetgr.o getpwent.o setlocale.o yplib.o

clean cleandir distclean:
	/bin/rm -f *.core ${AUXCLEAN} ${IMAGE} netbsd.INSTALL ${CBIN} \
		${CBIN}.mk ${CBIN}.cache *.o *.lo *.cro *.c

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
release: $(IMAGE) netbsd.INSTALL
	@mkdir -p $(RELEASEDIR)/installation/miniroot
	gzip -c -9 < $(.OBJDIR)/miniroot.fs \
		> $(RELEASEDIR)/installation/miniroot/miniroot.fs.gz
	@mkdir -p $(RELEASEDIR)/binary/kernels
.endif	# RELEASEDIR check

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
.include <bsd.sys.mk>
