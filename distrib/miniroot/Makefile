#	$NetBSD: Makefile,v 1.59 2002/04/26 15:50:37 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

.include <bsd.kernobj.mk>

ARCHDIR=	${.CURDIR}/../${MACHINE}/miniroot

CRUNCHBIN=	instbin
CRUNCHGEN_FLAGS= -d "${DBG}"
LISTS=		${.CURDIR}/list ${ARCHDIR}/list
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGE=		miniroot.fs
IMAGEDEPENDS=	${CRUNCHBIN} install.sub install.sh upgrade.sh \
		${KERNSRCDIR}/conf/osrelease.sh \
		${NETBSDSRCDIR}/etc/group ${NETBSDSRCDIR}/etc/master.passwd \
		${NETBSDSRCDIR}/etc/netconfig ${NETBSDSRCDIR}/etc/protocols \
		${NETBSDSRCDIR}/etc/services
PARSELISTENV=	ARCHDIR=${ARCHDIR:Q} \
		DISTRIBREV=${DISTRIBREV:Q} \
		DISTRIBVER=${DISTRIBVER:Q} \
		KERNOBJDIR=${KERNOBJDIR:Q}

.include "${ARCHDIR}/Makefile.inc"

IMAGEBUILT=	image.built
CLEANFILES+=	${IMAGEBUILT}
IMAGE_MD_POST?=	true

realall: check_DESTDIR .WAIT ${IMAGEBUILT}

release: check_RELEASEDIR .WAIT ${IMAGEBUILT}
	mkdir -p ${RELEASEDIR}/installation/miniroot
	gzip -c -9 < ${.OBJDIR}/miniroot.fs \
		> ${RELEASEDIR}/installation/miniroot/miniroot.fs.gz
	${MAKESUMS} -t ${RELEASEDIR}/installation/miniroot miniroot.fs.gz

${IMAGEBUILT}: ${IMAGE}
	${IMAGE_MD_POST} \
	&& touch ${IMAGEBUILT}

.include "${DISTRIBDIR}/common/Makefile.crunch"
.if defined(MAKEDEVTARGETS)
.include "${DISTRIBDIR}/common/Makefile.makedev"
.endif
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
