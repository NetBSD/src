#	$NetBSD: Makefile,v 1.36 2002/05/14 06:21:21 lukem Exp $
#
# boot.fs is the image for disk 1 of the two-set floppy based installation
# method.
#
# It is constructed by injecting the microroot filesystem `ramdisk.fs'
# into the md based kernel built from the INSTALL kernel configuration file.
#

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

IMAGE=		boot.fs
IMAGESIZE=	1440k
LISTS=		${.CURDIR}/list
MTREECONF=	${DISTRIBDIR}/common/mtree.dot
IMAGEENDIAN=	be
PRIMARYBOOT=	${DESTDIR}/usr/mdec/bootxx
IMAGEDEPENDS=	netbsd.ram.gz ${DESTDIR}/usr/mdec/boot ${PRIMARYBOOT}
IMAGEPOSTBUILD=	${INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} ${PRIMARYBOOT} /boot

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

IMAGE_RELEASEDIR=	installation/bootfs

MDSETTARGETS=		INSTALL ${RAMDISK} netbsd.ram
MDSET_RELEASEDIR=	installation/bootfs
MDSET_SUFFIXES.netbsd.ram=  aout create-aout

# conjure up a magic header that is accepted by all Sun PROMS;
# see sys/arch/sparc/stand/installboot/installboot.c for details.
#
SUN_MAGIC_HEADER='\01\03\01\07\060\200\0\07\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0'

create-aout=\
	${OBJCOPY} -O binary ${.TARGET:R} ${.TARGET}.raw && \
	( printf ${SUN_MAGIC_HEADER}; cat ${.TARGET}.raw ) > ${.TARGET}

CLEANFILES+=	netbsd.ram.aout.raw


FD?=		fd0
FD_RDEV=	/dev/r${FD}a

real-floppy:
	dd if=${IMAGE} of=${FD_RDEV} bs=32k


.include "${DISTRIBDIR}/common/Makefile.image"
.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
