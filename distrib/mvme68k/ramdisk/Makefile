#	$NetBSD: Makefile,v 1.11 2001/10/13 06:15:18 jmc Exp $

TOP=		${.CURDIR}/..

# This include just sets REV=XX
.include "${TOP}/Makefile.inc"

IMAGE=		ramdisk-${REV}.fs
CBIN=		rd_bin

TREE=		mtree.conf
LISTS=		list
CRUNCHCONF=	${CBIN}.conf

KERNEL=		${KERNOBJDIR}/RAMDISK/netbsd

MOUNT_POINT?=   /mnt
# DEV/RDEV file system device, CDEV/RDEV vnconfig device
VND?=           vnd0
VND_CDEV=       /dev/${VND}c
VND_CRDEV=      /dev/r${VND}c

CRUNCHGEN?=	crunchgen
MTREE?=		mtree

# These are all the parameters for the root fs: (384K)
# NOTE: If you change NBLKS, you *must* also edit the
#       rdroot entry in /etc/disktab
DISKTYPE=	rdroot
NBLKS=		1024

CLEANFILES=	netbsd-rd ${IMAGE}

netbsd-rd:	${IMAGE}
	cp ${KERNEL} netbsd-tmp
	mdsetimage netbsd-tmp ${IMAGE}
	-mv -f netbsd-tmp $@

${IMAGE}:	${TREE} ${LISTS} ${CBIN}
	dd if=/dev/zero of=${IMAGE} count=${NBLKS}
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	newfs -B be -m 0 -o space -i 2048 ${VND_CRDEV} ${DISKTYPE}
	mount ${VND_CDEV} ${MOUNT_POINT}
	${MTREE} -def ${.CURDIR}/${TREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    KERNOBJDIR=${KERNOBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${.CURDIR}/runlist.sh ${.CURDIR}/${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}

unconfig:
	-umount -f ${MOUNT_POINT}     
	-vnconfig -u ${VND_CDEV}      
	-/bin/rm -f ${IMAGE}

# Do not delete this if I change my mind and kill make...
.PRECIOUS: ${IMAGE}

CLEANFILES+= ${CBIN} ${CBIN}.c ${CBIN}.cache ${CBIN}.mk ${CBIN}.syms
CLEANFILES+= *.cro *_stub.o *_stub.c

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	${CRUNCHGEN} -D ${BSDSRCDIR} -L ${DESTDIR}/usr/lib ${CRUNCHCONF}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	${MAKE} -f ${CBIN}.mk all

# This is listed in rd_bin.conf but is built here.
${CBIN} : libhack.o

# Use stubs to eliminate some large stuff from libc
HACKSRC=${TOP}/../utils/libhack
.include "${HACKSRC}/Makefile.inc"

release:	netbsd-rd
	-mkdir -p ${RELEASEDIR}/installation/tapeimage
	gzip < netbsd-rd > ${RELEASEDIR}/installation/tapeimage/netbsd-rd.gz

clean cleandir distclean:
	-rm -f a.out core *.core *.o
	-rm -f ${CLEANFILES}

# Standard rules needed by the above...
.include <bsd.kernobj.mk>
.include <bsd.obj.mk>
