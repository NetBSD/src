#	$NetBSD: Makefile,v 1.53 2002/03/18 04:08:47 lukem Exp $

.include "${.CURDIR}/../Makefile.inc"
.include <bsd.own.mk>
.include <bsd.kernobj.mk>

ARCHDIR=	${.CURDIR}/../${MACHINE}/miniroot

CRUNCHBIN=	instbin
LISTS=		${.CURDIR}/list ${ARCHDIR}/list
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGE=		miniroot.fs
IMAGEDEPENDS=	${CRUNCHBIN} install.sub install.sh upgrade.sh \
		${KERNSRCDIR}/conf/osrelease.sh \
		${_SRC_TOP_}/etc/group ${_SRC_TOP_}/etc/master.passwd \
		${_SRC_TOP_}/etc/netconfig ${_SRC_TOP_}/etc/protocols \
		${_SRC_TOP_}/etc/services
PARSELISTENV=	ARCHDIR=${ARCHDIR:Q} \
		DISTRIBREV=${DISTRIBREV:Q} \
		DISTRIBVER=${DISTRIBVER:Q} \
		KERNOBJDIR=${KERNOBJDIR:Q}

.include "${ARCHDIR}/Makefile.inc"


all:	check_DESTDIR .WAIT image
	@true

release: check_RELEASEDIR .WAIT image
	mkdir -p ${RELEASEDIR}/installation/miniroot
	gzip -c -9 < ${.OBJDIR}/miniroot.fs \
		> ${RELEASEDIR}/installation/miniroot/miniroot.fs.gz
	sh ${DISTRIBDIR}/sets/makesums \
		-t ${RELEASEDIR}/installation/miniroot miniroot.fs.gz

image:	${IMAGE} image_md_post

image_md_post:
# nothing here; refer to machine-dependant ${ARCHDIR}/Makefile.inc


.include "${DISTRIBDIR}/common/Makefile.crunch"
.if defined(MAKEDEVTARGETS)
.include "${DISTRIBDIR}/common/Makefile.makedev"
.endif
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
