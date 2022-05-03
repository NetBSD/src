#	$NetBSD: Makefile,v 1.9 2022/05/03 10:50:42 jkoshy Exp $

HOSTLIB=	ctf

.include <bsd.hostinit.mk>

SRCS=		ctf_create.c \
		ctf_decl.c \
		ctf_error.c \
		ctf_hash.c \
		ctf_labels.c \
		ctf_lib.c \
		ctf_lookup.c \
		ctf_open.c \
		ctf_subr.c \
		ctf_types.c \
		ctf_util.c

OSNETDIR=	${.CURDIR}/../../external/cddl/osnet
OPENSOLARIS_DISTDIR= ${OSNETDIR}/dist
LIBELF_DIR=	${.CURDIR}/../../external/bsd/elftoolchain/dist/libelf

CPPFLAGS+=	-DCTF_OLD_VERSIONS

.ifndef NOCOMPATLIB
COMPATLIB_NO_LIB= yes # only the include files, not the library
.-include	"${TOOLDIR}/share/compat/defs.mk"
.endif

CPPFLAGS+=	-I${.CURDIR}/../compat \
		-I${OSNETDIR}/sys \
		-I${OSNETDIR}/include \
		-I${OPENSOLARIS_DISTDIR}/head \
		-I${OPENSOLARIS_DISTDIR}/common/ctf \
		-I${OPENSOLARIS_DISTDIR}/lib/libctf/common \
		-I${OPENSOLARIS_DISTDIR}/uts/common \
		-I${LIBELF_DIR} \
		-I${TOOLDIR}/include

BUILD_OSTYPE!=  uname -s

# Disable use of pre-compiled headers on Darwin.
.if ${BUILD_OSTYPE} == "Darwin"
CPPFLAGS+=	-no-cpp-precomp
.endif

# -D_FILE_OFFSET_BITS=64 produces a much more amenable `struct stat', and
# other file ops, on many systems, without changing function names.

CPPFLAGS+=	-DHAVE_NBTOOL_CONFIG_H=1 -D_FILE_OFFSET_BITS=64

.PATH:		${OPENSOLARIS_DISTDIR}/common/ctf
.PATH:		${OPENSOLARIS_DISTDIR}/lib/libctf/common

HOST_CPPFLAGS:=	${CPPFLAGS} ${HOST_CPPFLAGS}
CPPFLAGS:=	# empty

realinstall: install.host
install.host: ${TOOLDIR}/lib/libctf.a
${TOOLDIR}/lib/libctf.a:: libctf.a
	${_MKTARGET_INSTALL}
	mkdir -p ${TOOLDIR}/lib
	${HOST_INSTALL_FILE} -m ${BINMODE} libctf.a ${.TARGET}

.include "${.CURDIR}/../Makefile.nbincludes"
.include <bsd.hostlib.mk>
