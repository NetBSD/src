#	$NetBSD: Makefile,v 1.47 2002/02/12 01:38:17 lukem Exp $

.include "../../Makefile.inc"
.include <bsd.own.mk>
.include <bsd.kernobj.mk>

CBIN=		instbin
ARCHDIR=	${.CURDIR}/../${MACHINE}/miniroot

.if exists(${ARCHDIR}/Makefile.inc)
.include "${ARCHDIR}/Makefile.inc"
.endif

VND?=		vnd0
VND_DEV=	/dev/${VND}c
VND_RDEV=	/dev/r${VND}c
VND_GEOM?=
IMAGE?=		miniroot.fs
IMAGESIZE?=	14336 # 7MB in 512 byte blocks
NEWFSOPTS?=
MOUNT_POINT?=	/mnt
BOOTINSTALL?=	true
PRELABEL?=	true

.ifdef DISKTYPE
DISKTYPEARG=	-t ${DISKTYPE}
.endif

LISTS=		${.CURDIR}/list ${ARCHDIR}/list
CRUNCHCONF=	${CBIN}.conf
MTREECONF=	${.CURDIR}/mtree.conf
ICMD=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}

CRUNCHGEN?=	crunchgen
MTREE?=		mtree

.if !defined(DESTDIR) || ${DESTDIR} == ""
all:
	@echo setenv DESTDIR before making a miniroot!
	@false
.else

all: ${CBIN}
	dd if=/dev/zero of=${IMAGE} count=${IMAGESIZE}
	vnconfig ${DISKTYPEARG} -v -c ${VND} ${IMAGE} ${VND_GEOM}
	${PRELABEL}
	-newfs -m 0 -o space ${NEWFSOPTS} ${VND_RDEV} ${DISKTYPE}
	${BOOTINSTALL}
	mount -o async ${VND_DEV} ${MOUNT_POINT}
	${MTREE} -def ${MTREECONF} -p ${MOUNT_POINT}/ -u
	REV=${DISTRIBREV} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} ARCHDIR=${ARCHDIR} \
	    TARGDIR=${MOUNT_POINT} VND_RDEV=${VND_RDEV} \
	    KERNOBJDIR=${KERNOBJDIR} \
	    sh ${.CURDIR}/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${IMAGE}

${CBIN}.conf: ${LISTS}
	awk -f ${.CURDIR}/makeconf.awk CBIN=${CBIN} ${LISTS} > ${CBIN}.conf

${CBIN}.mk: .EXEC
${CBIN}.mk ${CBIN}.c: ${CRUNCHCONF}
	${CRUNCHGEN} -f -D ${.CURDIR}/../.. -L ${DESTDIR}/usr/lib ${CRUNCHCONF}

${CBIN}: ${CBIN}.mk ${CBIN}.c
	${MAKE} -f ${CBIN}.mk all

.if !defined(RELEASEDIR) || ${RELEASEDIR} == ""
release:
	@echo setenv RELEASEDIR first
	@false
.else
release: ${IMAGE}
	mkdir -p ${RELEASEDIR}/installation/miniroot
	gzip -c -9 < ${.OBJDIR}/miniroot.fs \
		> ${RELEASEDIR}/installation/miniroot/miniroot.fs.gz
	${.CURDIR}/../sets/makesums \
		-t ${RELEASEDIR}/installation/miniroot miniroot.fs.gz

.endif	# RELEASEDIR check
.endif	# DESTDIR check

clean cleandir distclean:
	/bin/rm -f core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache \
	${CBIN}.conf *.o *.cro *.c

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
