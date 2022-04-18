#	$NetBSD: Makefile,v 1.10 2022/04/18 19:40:06 jkoshy Exp $

.include <bsd.hostinit.mk>

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

ELFTOOLCHAIN_DIR=	${.CURDIR}/../../external/bsd/elftoolchain/dist
LIBELF_DIR=		${ELFTOOLCHAIN_DIR}/libelf
LIBDWARF_DIR=		${ELFTOOLCHAIN_DIR}/libdwarf

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
DWARFOBJ!=	cd ${.CURDIR}/../elftoolchain/libdwarf && ${PRINTOBJDIR}
ELFOBJ!=	cd ${.CURDIR}/../elftoolchain/libelf && ${PRINTOBJDIR}
LDADD+=		-L${CTFOBJ} -lctf
DPADD+=		${CTFOBJ}/libctf.a
LDADD+=		-L${DWARFOBJ} -ldwarf
DPADD+=		${DWARFOBJ}/libdwarf.a
LDADD+=		-L${ELFOBJ} -lelf
DPADD+=		${ELFOBJ}/libelf.a

.PATH:		${OPENSOLARIS_DISTDIR}/tools/ctf/common
.PATH:		${OPENSOLARIS_DISTDIR}/tools/ctf/cvt

.include "${.CURDIR}/../Makefile.nbincludes"
.include "${.CURDIR}/../Makefile.host"
