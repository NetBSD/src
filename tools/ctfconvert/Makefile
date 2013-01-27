#	$NetBSD: Makefile,v 1.4 2013/01/27 19:21:12 riastradh Exp $

NOMAN=	# defined

.include <bsd.own.mk>

HOSTPROGNAME=   ${_TOOL_PREFIX}ctfconvert
HOST_SRCDIR=    external/cddl/osnet/usr.bin/ctfconvert
#HOST_SRCS=	alist.c \
		ctf.c \
		ctfconvert.c \
		dwarf.c \
		fixup_tdescs.c \
		hash.c \
		iidesc.c \
		input.c \
		list.c \
		memory.c \
		merge.c \
		output.c \
		st_parse.c \
		stabs.c \
		stack.c \
		strtab.c \
		symbol.c \
		tdata.c \
		traverse.c \
		util.c

LIBELF_DIR=	${.CURDIR}/../../external/bsd/libelf/dist
LIBDWARF_DIR=	${.CURDIR}/../../external/bsd/libdwarf/dist

HOST_CPPFLAGS+=	-I${OSNETDIR}/sys \
		-I${OSNETDIR}/include \
		-I${OPENSOLARIS_DISTDIR} \
		-I${OPENSOLARIS_DISTDIR}/head \
		-I${OPENSOLARIS_DISTDIR}/tools/ctf/common \
		-I${OPENSOLARIS_DISTDIR}/tools/ctf/cvt \
		-I${OPENSOLARIS_DISTDIR}/uts/common \
		-I${LIBELF_DIR} \
		-I${LIBDWARF_DIR}

HOST_CPPFLAGS+=	-DHAVE_NBTOOL_CONFIG_H=1

OSNETDIR=	${.CURDIR}/../../external/cddl/osnet
OPENSOLARIS_DISTDIR= ${OSNETDIR}/dist

CTFOBJ!=	cd ${.CURDIR}/../libctf && ${PRINTOBJDIR}
DWARFOBJ!=	cd ${.CURDIR}/../libdwarf && ${PRINTOBJDIR}
ELFOBJ!=	cd ${.CURDIR}/../libelf && ${PRINTOBJDIR}
LDADD+=		-L${CTFOBJ} -lctf
LDADD+=		-L${DWARFOBJ} -ldwarf
LDADD+=		-L${ELFOBJ} -lelf

.PATH:		${OPENSOLARIS_DISTDIR}/tools/ctf/common
.PATH:		${OPENSOLARIS_DISTDIR}/tools/ctf/cvt

.include "${.CURDIR}/../Makefile.nbincludes"
.include "${.CURDIR}/../Makefile.host"
