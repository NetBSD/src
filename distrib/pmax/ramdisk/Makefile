#	$NetBSD: Makefile,v 1.23 2002/04/01 12:16:13 lukem Exp $

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.own.mk>

DISKIMAGE=	diskimage.tgz

IMAGE=		ramdisk.fs
IMAGESIZE=	6144b
MAKEFS_FLAGS=	-f 15

WARNS=		1

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	le
MAKEDEVTARGETS=	minimal pty0
IMAGEDEPENDS=	${CRUNCHBIN} \
		disktab.preinstall dot.hdprofile dot.profile \
		${CURDIR}/../miniroot/termcap.rcons \
		${DESTDIR}/usr/mdec/boot.pmax ${DESTDIR}/usr/mdec/bootxx_ffs \
		${_SRC_TOP_}/etc/group ${_SRC_TOP_}/etc/master.passwd \
		${_SRC_TOP_}/etc/netconfig ${_SRC_TOP_}/etc/protocols \
		${_SRC_TOP_}/etc/services

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o


${DISKIMAGE}: ${WORKBUILT} ${WORKSPEC}
	( cd ${WORKDIR}; \
	    GZIP=-9 ${PAX} -w -d -z -N ${DESTDIR}/etc -M < ${WORKSPEC} \
	) > ${.TARGET}.tmp \
	&& mv ${.TARGET}.tmp ${.TARGET}

CLEANFILES+=	${DISKIMAGE} ${DISKIMAGE}.tmp



realall: ${IMAGE} .WAIT ${DISKIMAGE}

release: check_RELEASEDIR .WAIT ${DISKIMAGE}
	-mkdir -p ${RELEASEDIR}/installation/netboot
	cp -p ${DISKIMAGE} ${RELEASEDIR}/installation/netboot


.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
