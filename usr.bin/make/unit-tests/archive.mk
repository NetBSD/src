# $NetBSD: archive.mk,v 1.3 2020/08/16 14:39:50 rillig Exp $
#
# Very basic demonstration of handling archives, based on the description
# in PSD.doc/tutorial.ms.

ARCHIVE=	libprog.${EXT.a}
FILES=		archive.${EXT.mk} modmisc.${EXT.mk} varmisc.mk

EXT.a=		a
EXT.mk=		mk

MAKE_CMD=	${.MAKE} -f ${MAKEFILE}
RUN?=		@set -eu;

all:
	${RUN} ${MAKE_CMD} remove-archive
	${RUN} ${MAKE_CMD} create-archive
	${RUN} ${MAKE_CMD} list-archive
	${RUN} ${MAKE_CMD} depend-on-existing-member
	${RUN} ${MAKE_CMD} depend-on-nonexistent-member
	${RUN} ${MAKE_CMD} remove-archive

create-archive: ${ARCHIVE}
${ARCHIVE}: ${ARCHIVE}(${FILES})
	ar cru ${.TARGET} ${.OODATE}
	ranlib ${.TARGET}

list-archive: ${ARCHIVE}
	ar t ${.ALLSRC}

depend-on-existing-member: ${ARCHIVE}(archive.mk)
	${RUN} echo $@

depend-on-nonexistent-member: ${ARCHIVE}(nonexistent.mk)
	${RUN} echo $@

remove-archive:
	rm -f ${ARCHIVE}
