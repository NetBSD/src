#	$NetBSD: Makefile,v 1.30.2.1 2014/08/10 06:48:45 tls Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

.include "${.CURDIR}/../instfs/Makefile.instfs"
IMAGE=		miniroot.fs
NUMCYLS=	12 	# size of image in MB, tune this if we need more space
SECSPERCYL=	2048
CYLSIZE=	$$(( ${SECSPERCYL} * 512 ))
IMAGESIZE=	$$(( ${NUMCYLS} * ${CYLSIZE} ))
IMAGEPOSTBUILD=	\
	${TOOL_INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} \
		${DESTDIR}/usr/mdec/bootblk && \
	echo "Creating disklabel" && \
	printf 'V nsect %d\nV nhead 1\nV rpm 7200\nV pcyl %d\nV ncyl %d\na 0 %d/0/0\nc 0 %d/0/0\nd 0 %d/0/0\nW\nL\nP\n' \
		${SECSPERCYL} ${NUMCYLS} ${NUMCYLS} ${NUMCYLS} ${NUMCYLS} ${NUMCYLS} | \
	${TOOL_SUNLABEL} -nq ${IMAGE}

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
	${TOOL_GZIP_N} -9c ${.ALLSRC} > ${.TARGET}

clean:	localclean

localclean:
	rm -rf sysinst

CLEANFILES+=	${CRUNCHBIN} netbsd.gz

.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
