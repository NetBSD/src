#	$NetBSD: Makefile,v 1.19 2002/03/12 06:26:35 lukem Exp $

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.own.mk>

DISKIMAGE=	diskimage.tgz

IMAGE=		ramdisk.fs
IMAGESIZE=	6144b

WARNS=		1

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	le
MAKEDEVTARGETS=	minimal pty0
IMAGEDEPENDS=	${CRUNCHBIN} \
		disktab.preinstall dot.profile dot.hdprofile \
		${CURDIR}/../miniroot/termcap.rcons \
		${DESTDIR}/usr/mdec/bootxx_ffs ${DESTDIR}/usr/mdec/boot.pmax \
		${_SRC_TOP_}/etc/group ${_SRC_TOP_}/etc/master.passwd \
		${_SRC_TOP_}/etc/protocols ${_SRC_TOP_}/etc/netconfig \
		${_SRC_TOP_}/etc/services


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
