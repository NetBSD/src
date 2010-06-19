#	$NetBSD: Makefile,v 1.2 2010/06/19 23:11:10 riz Exp $

.include <bsd.own.mk>

HOSTLIB=dwarf

SRCS=	dwarf_abbrev.c		\
	dwarf_attr.c		\
	dwarf_attrval.c		\
	dwarf_cu.c		\
	dwarf_dealloc.c		\
	dwarf_die.c		\
	dwarf_dump.c		\
	dwarf_errmsg.c		\
	dwarf_errno.c		\
	dwarf_finish.c		\
	dwarf_form.c		\
	dwarf_init.c		\
	dwarf_loc.c

LIBDWARF_DIR=	${.CURDIR}/../../external/bsd/libdwarf/dist
LIBELF_DIR=	${.CURDIR}/../../external/bsd/libelf/dist

INCS=		dwarf.h libdwarf.h
INCSDIR=	/usr/include

.ifndef NOCOMPATLIB
COMPATOBJ!=     cd ${.CURDIR}/../compat && ${PRINTOBJDIR}
CPPFLAGS+=	-I${COMPATOBJ}
.endif

CPPFLAGS+=	-I${.CURDIR}/../compat -I${LIBDWARF_DIR} -I${LIBELF_DIR}

BUILD_OSTYPE!=  uname -s

# Disable use of pre-compiled headers on Darwin.
.if ${BUILD_OSTYPE} == "Darwin"
CPPFLAGS+=	-no-cpp-precomp
.endif

# -D_FILE_OFFSET_BITS=64 produces a much more amenable `struct stat', and
# other file ops, on many systems, without changing function names.

CPPFLAGS+=	-DHAVE_NBTOOL_CONFIG_H=1 -D_FILE_OFFSET_BITS=64

.PATH:		${LIBDWARF_DIR}

HOST_CPPFLAGS:=	${CPPFLAGS}
CPPFLAGS:=	# empty

.include "${.CURDIR}/../Makefile.nbincludes"
.include <bsd.hostlib.mk>
