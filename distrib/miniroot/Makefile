#	$NetBSD: Makefile,v 1.23 1999/10/18 20:13:55 pk Exp $

.include <bsd.own.mk>

REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s

CBIN=		instbin
ARCHDIR=	${.CURDIR}/../${MACHINE}/miniroot

.if exists(${ARCHDIR}/Makefile.inc)
.include "${ARCHDIR}/Makefile.inc"
.endif

VND?=		vnd0c
VND_DEV=	/dev/${VND}
VND_RDEV=	/dev/r${VND}
VND_GEOM?=
IMAGE?=		miniroot-${REV}.fs
IMAGESIZE?=	12288 # 6MB in 512 byte blocks
NEWFSOPTS?=
MOUNT_POINT?=	/mnt
BOOTINSTALL?=	true
PRELABEL?=	true

.ifdef DISKTYPE
DISKTYPEARG=	-t ${DISKTYPE}
.endif

LISTS=		${.CURDIR}/list ${ARCHDIR}/list
CRUNCHCONF=	${CBIN}.conf
MTREE=		${.CURDIR}/mtree.conf
ICMD=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY}	\
			-o root -g wheel -m ${NONBINMODE} 

.ifndef DESTDIR
all:
	@echo setenv DESTDIR before making a miniroot!
	@false
.else

all: ${CBIN}
	dd if=/dev/zero of=${IMAGE} count=${IMAGESIZE}
	vnconfig ${DISKTYPEARG} -v -c ${VND_DEV} ${IMAGE} ${VND_GEOM}
	${PRELABEL}
	-newfs -m 0 -o space ${NEWFSOPTS} ${VND_RDEV} ${DISKTYPE}
	${BOOTINSTALL}
	mount -o async ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${MTREE} -p ${MOUNT_POINT}/ -u
	REV=${REV} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} ARCHDIR=${ARCHDIR} \
	    TARGDIR=${MOUNT_POINT} VND_RDEV=${VND_RDEV} \
	    sh ${.CURDIR}/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_DEV}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${IMAGE}

${CBIN}.conf: ${LISTS}
	awk -f ${.CURDIR}/makeconf.awk CBIN=${CBIN} ${LISTS} > ${CBIN}.conf

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	crunchgen -D ${.CURDIR}/../.. -L ${DESTDIR}/usr/lib ${CRUNCHCONF}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	make -f ${CBIN}.mk all

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
release: $(IMAGE)
	gzip -c -9 < $(.OBJDIR)/miniroot-${REV}.fs \
		> $(RELEASEDIR)/installation/miniroot/miniroot-${REV}.fs.gz
.endif	# RELEASEDIR check
.endif	# DESTDIR check

clean cleandir distclean:
	/bin/rm -f core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache \
	${CBIN}.conf *.o *.cro *.c

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
