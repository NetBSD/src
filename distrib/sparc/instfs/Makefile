#	$NetBSD: Makefile,v 1.8 2002/05/09 11:12:48 lukem Exp $
#
# instfs.tgz is the filesystem image for disk 2 of the floppy based
# installation method.
# It is constructed by tarring up the contents of the miniroot with
# the exeption of the kernel and boot program.
#

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

IMAGETAR=		instfs.tgz
IMAGETAR_RELEASEDIR=	installation/bootfs

MINIROOTDIR!=	cd ${.CURDIR}/../../miniroot && ${PRINTOBJDIR}
WORKDIR=	${MINIROOTDIR}/work
WORKBUILT=	${MINIROOTDIR}/work.built
WORKSPEC=	instfs.spec

${WORKSPEC}: ${MINIROOTDIR}/work.spec
	-rm -f ${.TARGET} ${.TARGET}.tmp
	egrep -v '^\./(netbsd|boot)($$|[ \t])' ${.ALLSRC} > ${.TARGET}.tmp \
	&& mv ${.TARGET}.tmp ${.TARGET}

CLEANFILES+=	${WORKSPEC} ${WORKSPEC}.tmp

${WORKBUILT}:
	@echo "Need ../../miniroot built first"
	@false

.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
