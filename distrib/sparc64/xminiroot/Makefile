#	$NetBSD: Makefile,v 1.11 2002/01/20 05:57:10 mrg Exp $

TOP=		${.CURDIR}/..
MINIROOT=	${TOP}/../miniroot

.include "${TOP}/Makefile.inc"

.include <bsd.own.mk>	# So we use /etc/mk.conf.
.include <bsd.kernobj.mk>

HACKSRC=	${TOP}/../utils/libhack
SRCROOT=	${TOP}/../..
WARNS=1
KERN?=		${KERNOBJDIR}/INSTALL/netbsd

AUXDEPENDS= 	dot.profile disktab.preinstall termcap.mini \
		dist/base_obsolete dist/comp_obsolete \
		dist/games_obsolete dist/man_obsolete dist/misc_obsolete \
		dist/secr_obsolete dist/xbase_obsolete dist/xserver_obsolete
AUXCLEAN=	list ${CBIN}.conf dot.profile

CBIN=		ramdiskbin


CSIZE?=		15360
IMAGE?=		miniroot.fs
MDEC=		${DESTDIR}/usr/mdec
MOUNT_POINT?=	${.OBJDIR}/mountpoint

LISTS=		list
CRUNCHCONF=	${CBIN}.conf
MTREECONF=	mtree.conf

CRUNCHGEN?=	crunchgen
MDSETIMAGE?=	mdsetimage
MTREE?=		mtree
DISKTYPE?=	miniroot

MAKEFSOPTS?=	-o optimization=space,minfree=1,nsectors=1,ntracks=${CSIZE},bsize=16k,fsize=2k

all: netbsd.INSTALL ${IMAGE}

netbsd.INSTALL: ${KERN} ${IMAGE}
	cp ${KERN} netbsd.tmp
	${MDSETIMAGE} -v netbsd.tmp ${IMAGE}
	${STRIP} netbsd.tmp
	mv netbsd.tmp ${.TARGET}


${IMAGE}: ${CBIN} ${AUXDEPENDS} ${MTREECONF} ${LISTS} dot.profile
	@echo "Building miniroot tree..."
	-rm -rf ${MOUNT_POINT}
	mkdir -m 755 ${MOUNT_POINT}
	${MTREE} -def ${.CURDIR}/${MTREECONF} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    KERNOBJDIR=${KERNOBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${LISTS}
	@echo "Creating miniroot image..."
	-rm -f ${IMAGE}
	${MAKEFS} -t ffs -B be -s ${CSIZE}b ${MAKEFSOPTS} \
		${IMAGE}.tmp ${MOUNT_POINT} && mv ${IMAGE}.tmp ${IMAGE}
	# use sunlabel?
	#-disklabel -f ${.CURDIR}/disktab -rw ${VND_CDEV} ${DISKTYPE}
	@echo "installing new bootblocks"
	dd if=${MDEC}/bootblk of=${IMAGE} bs=512 count=15 conv=notrunc seek=1

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
.include <bsd.prog.mk>
