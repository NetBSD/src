#	$NetBSD: Makefile.inc,v 1.17 2001/10/10 05:13:54 enami Exp $ 

# TOP is assumed to be defined by Makefile including this one.

CBIN=		instbin
COMMONDIR=	${TOP}/inst-common

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
IMAGE?=		inst.fs
MDEC=		${DESTDIR}/usr/mdec

LISTS=		${COMMONDIR}/list ${.CURDIR}/list
CRUNCHCONF=	${COMMONDIR}/${CBIN}.conf
MTREECONF=	${COMMONDIR}/mtree.conf
KERNEL?=	${KERNOBJDIR}/INSTALL/netbsd

CRUNCHGEN?=	crunchgen

DISKTYPE=	boot3
RAMDISKTYPE=	rd

all: netbsd.gz
	dd if=/dev/zero of=${IMAGE} bs=18k count=80
	disklabel -r -w ${.OBJDIR}/${IMAGE} boot3 2>/dev/null
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	newfs -B le -b 8192 -f 1024 -O -m 0 -o space -i 204800 -c 80 \
	    ${VND_RDEV} ${DISKTYPE}
	mount ${VND_DEV} ${MOUNT_POINT}
	cp netbsd.gz ${MOUNT_POINT}/netbsd.gz
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}
	-yes|bim -c init -c "add ${MDEC}/boot boot" -c "default 0" \
	    -c "exit" ${.OBJDIR}/${IMAGE} 2>/dev/null >/dev/null

ramdisk.fs: ${CBIN} ${LISTS}
.ifndef SD
	dd if=/dev/zero of=${.TARGET} bs=128k count=16
	vnconfig -t ${RAMDISKTYPE} -v -c ${VND_CDEV} ${.TARGET}
	disklabel -rw ${VND_CDEV} ${RAMDISKTYPE}
	newfs -B le -b 8192 -f 1024 -O -m 0 -o space -i 7168 \
	    ${VND_RDEV} ${RAMDISKTYPE}
	mount ${VND_DEV} ${MOUNT_POINT}
.else
	newfs -B le -b 8192 -f 1024 -O -m 0 -o space -i 7168 -s 4096 \
		-t 1 -u 256 /dev/r${SD}
	mount /dev/${SD} ${MOUNT_POINT}
.endif
	mtree -def ${MTREECONF} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    KERNOBJDIR=${KERNOBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
.ifndef SD
	vnconfig -u ${VND_CDEV}
.else
	dd if=/dev/${SD} of=${.TARGET} bs=128k count=16
.endif


unconfig:
	umount -f ${MOUNT_POINT}
	vnconfig -u ${VND_DEV}
	/bin/rm -f ${IMAGE}

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	${CRUNCHGEN} -D ${TOP}/../../.. -L ${DESTDIR}/usr/lib ${CRUNCHCONF}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	${MAKE} -f ${CBIN}.mk all

netbsd.gz: ramdisk.fs
	cp ${KERNEL} /var/tmp/$$$$; \
		mdsetimage -v /var/tmp/$$$$ ramdisk.fs; \
		strip /var/tmp/$$$$; \
		gzip -9 </var/tmp/$$$$ >netbsd.gz; \
		rm /var/tmp/$$$$

clean cleandir distclean:
	/bin/rm -f core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache *.o *.cro *.c
	/bin/rm -f netbsd*

.include <bsd.kernobj.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
