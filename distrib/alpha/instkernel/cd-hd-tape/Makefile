#	$NetBSD: Makefile,v 1.3 1999/03/22 08:44:57 ross Exp $

IMAGE=		cdhdtape
IMAGESIZE=	5760
NFILES=		3

INSTKERNEL!=cd ${.CURDIR}/../instkernel; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.gz\n" | \
	${MAKE} -s -f-

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
KERN?=		netbsd-xxx
MDEC=		${DESTDIR}/usr/mdec
FSTMP=		ustar.tmp

PRIMARYBOOT=	${DESTDIR}/usr/mdec/bootxx
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
	vnconfig -v -c ${VND_CDEV} ${IMAGE}	\
		512/18/2/$$((${IMAGESIZE} / (18 * 2)))
	dd bs=8k seek=1 if=${FSTMP} of=${VND_CDEV}
	@echo not really doing rm ${FSTMP}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	@echo "installing new bootblocks"
	${MDEC}/installboot -b 17 -v	\
		${SECONDARYBOOT} ${PRIMARYBOOT} ${VND_CRDEV}
	vnconfig -u ${VND_CDEV}

netbsd: ${INSTKERNEL}
	cp ${INSTKERNEL} netbsd

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_CDEV}
	-/bin/rm -f ${IMAGE}

clean cleandir distclean:
	/bin/rm -f *.core ${CLEANFILES}

.include <bsd.own.mk>
.include <bsd.obj.mk>
