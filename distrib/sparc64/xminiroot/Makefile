#	$NetBSD: Makefile,v 1.26.30.1 2009/05/13 19:17:55 jym Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

.include "${.CURDIR}/../instfs/Makefile.instfs"
IMAGE=			miniroot.fs
IMAGESIZE=		8m
IMAGEPOSTBUILD=		${TOOL_INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} \
			    ${DESTDIR}/usr/mdec/bootblk
IMAGE_RELEASEDIR=	installation/miniroot

CRUNCHBINDIR!=		cd ${INSTFSDIR} && ${PRINTOBJDIR}
LISTS+=			${.CURDIR}/list.miniroot
IMAGEDEPENDS+=		netbsd.gz

${CRUNCHBIN}: ${CRUNCHBINDIR}/${CRUNCHBIN}
	mkdir -p sysinst
	cp -p ${CRUNCHBINDIR}/sysinst/sysinstmsgs.* sysinst
	cp ${.ALLSRC} ${.TARGET}

netbsd.gz: ${KERNOBJDIR}/GENERIC/netbsd
	-rm -f ${.TARGET}
	gzip -9nc ${.ALLSRC} > ${.TARGET}

clean:	localclean

localclean:
	rm -rf sysinst

CLEANFILES+=	${CRUNCHBIN} netbsd.gz

.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
