#	$NetBSD: Makefile,v 1.8 2002/03/05 21:23:28 thorpej Exp $

.include <bsd.own.mk>

IMAGE=		cdhdtape
IMAGESIZE=	5760
NFILES=		3

INSTKERNELDIR!=	cd ${.CURDIR}/../instkernel && ${PRINTOBJDIR}
INSTKERNEL=	${INSTKERNELDIR}/netbsd.gz

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
KERN?=		netbsd-xxx
MDEC=		${DESTDIR}/usr/mdec
FSTMP=		ustar.tmp

PRIMARYBOOT=	${DESTDIR}/usr/mdec/bootxx.old
SECONDARYBOOT=	${DESTDIR}/usr/mdec/boot

DISKTYPE=	floppy
METANAME!=	printf USTAR.volsize.%o ${IMAGESIZE}

CLEANFILES+=	netbsd ${FSTMP} ${IMAGE} tmp tmp.gz USTAR.volsize.*

all:	netbsd
	@echo Making CD/tape image
	cp /dev/null ${METANAME}
	cp /dev/null tapelabel
	tar cvf ${FSTMP} ${SECONDARYBOOT} tapelabel ${METANAME} netbsd
	@rm tapelabel
	@echo not really doing rm ${METANAME}
	@ echo ''
	@ls -l ${FSTMP} | (read mode links uid gid size junk;	\
		bytes=$$((${IMAGESIZE} * 512 - $$size - 8 * 1024));\
		echo There are $$bytes \($$(($$bytes / 1024))K\) bytes free)
	@ echo ''
	dd if=/dev/zero of=${IMAGE} count=${IMAGESIZE}
	vnconfig -v -c ${VND} ${IMAGE}	\
		512/18/2/$$((${IMAGESIZE} / (18 * 2)))
	dd bs=8k seek=1 if=${FSTMP} of=${VND_CDEV}
	@echo not really doing rm ${FSTMP}
	disklabel -rw ${VND} ${DISKTYPE}
	@echo "installing new bootblocks"
	${MDEC}/installboot.old -b 17 -v	\
		${SECONDARYBOOT} ${PRIMARYBOOT} ${VND_CRDEV}
	vnconfig -u ${VND}

netbsd: ${INSTKERNEL}
	cp ${INSTKERNEL} netbsd

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND}
	-/bin/rm -f ${IMAGE}

clean cleandir distclean:
	/bin/rm -f *.core ${CLEANFILES}

# nothing to do here
depend includes lint regress tags:

.include <bsd.obj.mk>
