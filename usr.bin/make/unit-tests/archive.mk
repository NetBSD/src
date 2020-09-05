# $NetBSD: archive.mk,v 1.7 2020/09/05 06:20:51 rillig Exp $
#
# Very basic demonstration of handling archives, based on the description
# in PSD.doc/tutorial.ms.
#
# This test aims at covering the code, not at being an introduction to
# archive handling. That's why it is more complicated and detailed than
# strictly necessary.

ARCHIVE=	libprog.a
FILES=		archive.mk modmisc.mk varmisc.mk

MAKE_CMD=	${.MAKE} -f ${MAKEFILE}
RUN?=		@set -eu;

all:
.if ${.PARSEDIR:tA} != ${.CURDIR:tA}
	@cd ${MAKEFILE:H} && cp ${FILES} t*.mk ${.CURDIR}
.endif
# The following targets are run in sub-makes to ensure that they get the
# current state of the filesystem right, since they creating and removing
# files.
	${RUN} ${MAKE_CMD} remove-archive
	${RUN} ${MAKE_CMD} create-archive
	${RUN} ${MAKE_CMD} list-archive
	${RUN} ${MAKE_CMD} list-archive-wildcard
	${RUN} ${MAKE_CMD} depend-on-existing-member
	${RUN} ${MAKE_CMD} depend-on-nonexistent-member
	${RUN} ${MAKE_CMD} remove-archive

create-archive: ${ARCHIVE}

# The indirect references with the $$ cover the code in Arch_ParseArchive
# that calls Var_Parse.  It's an esoteric scenario since at the point where
# Arch_ParseArchive is called, the dependency line is already fully expanded.
#
${ARCHIVE}: $${:Ulibprog.a}(archive.mk modmisc.mk $${:Uvarmisc.mk})
	ar cru ${.TARGET} ${.OODATE}
	ranlib ${.TARGET}

list-archive: ${ARCHIVE}
	ar t ${.ALLSRC}

# XXX: I had expected that this dependency would select all *.mk files from
# the archive.  Instead, the globbing is done in the current directory.
# To prevent an overly long file list, the pattern is restricted to [at]*.mk.
list-archive-wildcard: ${ARCHIVE}([at]*.mk)
	${RUN} printf '%s\n' ${.ALLSRC:O:@member@${.TARGET:Q}': '${member:Q}@}

depend-on-existing-member: ${ARCHIVE}(archive.mk)
	${RUN} echo $@

depend-on-nonexistent-member: ${ARCHIVE}(nonexistent.mk)
	${RUN} echo $@

remove-archive:
	rm -f ${ARCHIVE}
