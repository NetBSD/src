#	$NetBSD: Makefile,v 1.7.2.3 2000/03/01 00:05:40 he Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
IMAGE=		ramdisk-${REV}.fs

AUXTARGETS=	install.sh upgrade.sh start.sh
AUXCLEAN=	${AUXTARGETS}

CBIN=		ramdiskbin

MOUNT_POINT?=	/mnt
# DEV/RDEV file system device, CDEV/CRDEV vnconfig device
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
MDEC=		${DESTDIR}/usr/mdec

LISTS=		list
CRUNCHCONF=	${CBIN}.conf
MTREE=		mtree.conf

# CYLS must be a multiple of 8 on arm32 (??)
# TOTALSECT must match options MINIROOTSIZE in the *INST kernels.
CYLS=		80
SECS=		15
TRKS=		3
TOTALSECT !=	expr ${CYLS} '*' ${SECS} '*' ${TRKS}
DISKTYPE=	install-ramdisk
DISKTAB=	./{DISKTYPE}.disktab
GEOM=		512/${SECS}/${TRKS}/${CYLS}
INO_BYTES=	5120

install.sh: install.tmpl
	sed "s/@@VERSION@@/${VER}/" < ${.ALLSRC} > ${.TARGET}

upgrade.sh: upgrade.tmpl
	sed "s/@@VERSION@@/${VER}/" < ${.ALLSRC} > ${.TARGET}

start.sh: start.tmpl
	sed "s/@@VERSION@@/${VER}/" < ${.ALLSRC} > ${.TARGET}

all: ${AUXTARGETS} ${CBIN} 
	dd if=/dev/zero of=${IMAGE} count=${TOTALSECT}
	vnconfig -v -c ${VND_CDEV} ${IMAGE} ${GEOM}
	echo "${DISKTYPE}:ty=simulated:se#512:nt#${TRKS}:ns#${SECS}:nc#${CYLS}:pa#${TOTALSECT}:oa#0:ba#4096:fa#512:ta=4.2BSD:pc#${TOTALSECT}:oc#0:" > ${DISKTAB}
	disklabel -rw -f ${DISKTAB} ${VND_CDEV} ${DISKTYPE}
	disklabel -W ${VND_CDEV}
	newfs -B le -m 0 -o space -i ${INO_BYTES} -c ${CYLS} ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${.CURDIR}/${MTREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
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

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	crunchgen -D ${TOP}/../.. -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	make -f ${CBIN}.mk all

clean cleandir distclean:
	/bin/rm -f ${AUXCLEAN} *.core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache *.o *.lo *.c

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
release: $(IMAGE)
	gzip -c -9 < $(.OBJDIR)/${IMAGE} \
		> $(RELEASEDIR)/installation/misc/${IMAGE}.gz
.for instkern in A7INST CATSINST RPCINST SHARKINST
	cp ../../../sys/arch/arm32/compile/${instkern}/netbsd netbsd.tmp
	mdsetimage -v netbsd.tmp ${IMAGE}
	gzip -c -9 < netbsd.tmp \
		> $(RELEASEDIR)/installation/kernel/netbsd.${instkern}.gz
.endfor # A7INST CATSINST RPCINST SHARKINST
.endif  # RELEASEDIR check

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
