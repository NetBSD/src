#	$NetBSD: Makefile,v 1.20 2002/05/08 23:11:57 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

.include "${.CURDIR}/../instfs/Makefile.instfs"
IMAGE=			miniroot.fs
IMAGESIZE=		7m
IMAGEPOSTBUILD=		${INSTALLBOOT} -m ${MACHINE} ${IMAGE} \
			    ${DESTDIR}/usr/mdec/bootblk
IMAGE_RELEASEDIR=	installation/miniroot

CRUNCHBINDIR!=		cd ${INSTFSDIR} && ${PRINTOBJDIR}
LISTS+=			${.CURDIR}/list.miniroot
IMAGEDEPENDS+=		netbsd.gz

${CRUNCHBIN}: ${CRUNCHBINDIR}/${CRUNCHBIN}
	cp ${.ALLSRC} ${.TARGET}

netbsd.gz: ${KERNOBJDIR}/GENERIC/netbsd
	-rm -f ${.TARGET}
	gzip -9c ${.ALLSRC} > ${.TARGET}

CLEANFILES+=	${CRUNCHBIN} netbsd.gz

.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
