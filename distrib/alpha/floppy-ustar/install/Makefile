#	$NetBSD: Makefile,v 1.3 1998/10/05 04:30:16 ross Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE1=		disk1of2
IMAGE2=		disk2of2
KERN=	${.CURDIR}/../../../../sys/arch/alpha/compile/INSTALL-FLOPPY/netbsd

# TOP is assumed to be defined by Makefile including this one.

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

LISTS=		${.CURDIR}/list
RAMDISK	!=cd $(.CURDIR)/../md_install/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/md_install-${REV}.fs\n" | \
	${MAKE} -s -f-

DISKTYPE=	floppy

CLEANFILES+=	netbsd ${FSTMP}

all:	netbsd
	@echo Making disk number one.
	tar cvf ${FSTMP} ${SECONDARYBOOT} netbsd
	@ls -l ${FSTMP} | (read mode links uid gid size junk;	\
		bytes=$$((2880 * 512 * 2 - $$size - 8 * 1024));		\
		echo There are $$bytes \($$(($$bytes / 1024))K\) bytes free\
			on disk 2.)
	dd if=/dev/zero of=${IMAGE1} count=2880
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE1}
	dd bs=8k seek=1 count=179 if=${FSTMP} of=${VND_CDEV}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	@echo "installing new bootblocks"
	${MDEC}/installboot -b 17 -v	\
		${SECONDARYBOOT} ${PRIMARYBOOT} ${VND_CRDEV}
	vnconfig -u ${VND_CDEV}
	@echo Making disk number two.
	echo USTARFS 2 > ${IMAGE2}
	(echo 'Prototype image created on: ';	\
		echo -n '     '; date;		\
		echo -n '     '; date -u) >> ${IMAGE2}
	(echo Hash value of corresponding volume:; md5 ${IMAGE1}) >> ${IMAGE2}
	dd bs=8k seek=1 skip=179 conv=sync if=${FSTMP} of=${IMAGE2}
	@rm ${FSTMP}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_CDEV}
	-/bin/rm -f ${IMAGE1} ${IMAGE2}

netbsd: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd.tmp
	mdsetimage -v netbsd.tmp ${RAMDISK}
	strip netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

clean cleandir:
	/bin/rm -f *.core ${IMAGE1} ${IMAGE2} ${CLEANFILES}

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
