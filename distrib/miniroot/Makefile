#	$NetBSD: Makefile,v 1.66 2008/09/19 17:23:32 tsutsui Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

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
		${NETBSDSRCDIR}/distrib/common/services
PARSELISTENV=	ARCHDIR=${ARCHDIR:Q} \
		DISTRIBREV=${DISTRIBREV:Q} \
		DISTRIBVER=${DISTRIBVER:Q} \
		KERNOBJDIR=${KERNOBJDIR:Q}

IMAGE_RELEASEDIR=	installation/miniroot

.include "${ARCHDIR}/Makefile.inc"

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

.include "${DISTRIBDIR}/common/Makefile.crunch"
.if defined(MAKEDEVTARGETS)
.include "${DISTRIBDIR}/common/Makefile.makedev"
.endif
.include "${DISTRIBDIR}/common/Makefile.image"

release:: check_RELEASEDIR .WAIT ${IMAGE}.gz
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/${IMAGE_RELEASEDIR} ${IMAGE}.gz

.include <bsd.prog.mk>
