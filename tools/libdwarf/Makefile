#	$NetBSD: Makefile,v 1.8 2018/05/01 00:04:36 christos Exp $

HOSTLIB=dwarf

.include <bsd.init.mk>

SRCS= \
	dwarf_abbrev.c	\
	dwarf_arange.c	\
	dwarf_attr.c	\
	dwarf_attrval.c	\
	dwarf_cu.c	\
	dwarf_dealloc.c	\
	dwarf_die.c	\
	dwarf_dump.c	\
	dwarf_errmsg.c	\
	dwarf_finish.c	\
	dwarf_form.c	\
	dwarf_frame.c	\
	dwarf_init.c	\
	dwarf_lineno.c	\
	dwarf_loclist.c	\
	dwarf_macinfo.c	\
	dwarf_pro_arange.c	\
	dwarf_pro_attr.c	\
	dwarf_pro_die.c	\
	dwarf_pro_expr.c	\
	dwarf_pro_finish.c	\
	dwarf_pro_frame.c	\
	dwarf_pro_init.c	\
	dwarf_pro_lineno.c	\
	dwarf_pro_macinfo.c	\
	dwarf_pro_reloc.c	\
	dwarf_pro_sections.c	\
	dwarf_ranges.c	\
	dwarf_reloc.c	\
	dwarf_seterror.c	\
	dwarf_str.c	\
	libdwarf.c	\
	libdwarf_abbrev.c	\
	libdwarf_arange.c	\
	libdwarf_attr.c	\
	libdwarf_die.c	\
	libdwarf_elf_access.c	\
	libdwarf_elf_init.c	\
	libdwarf_error.c	\
	libdwarf_frame.c	\
	libdwarf_info.c	\
	libdwarf_init.c	\
	libdwarf_lineno.c	\
	libdwarf_loc.c	\
	libdwarf_loclist.c	\
	libdwarf_macinfo.c	\
	libdwarf_nametbl.c	\
	libdwarf_ranges.c	\
	libdwarf_reloc.c	\
	libdwarf_rw.c	\
	libdwarf_sections.c	\
	libdwarf_str.c

ELFTOOLCHAIN_DIR=${.CURDIR}/../../external/bsd/elftoolchain/dist
LIBDWARF_DIR=	${ELFTOOLCHAIN_DIR}/libdwarf
LIBELF_DIR=	${ELFTOOLCHAIN_DIR}/libelf
COMMON_DIR=	${ELFTOOLCHAIN_DIR}/common

INCS=		dwarf.h libdwarf.h
INCSDIR=	/usr/include

.ifndef NOCOMPATLIB
COMPATLIB_NO_LIB= yes # only the include files, not the library
.-include	"${TOOLDIR}/share/compat/defs.mk"
.endif

CPPFLAGS+=	-I${.CURDIR}/../compat -I${LIBDWARF_DIR} -I${LIBELF_DIR}
CPPFLAGS+=	-I${COMMON_DIR}

BUILD_OSTYPE!=  uname -s

# Disable use of pre-compiled headers on Darwin.
.if ${BUILD_OSTYPE} == "Darwin"
CPPFLAGS+=	-no-cpp-precomp
.endif

# -D_FILE_OFFSET_BITS=64 produces a much more amenable `struct stat', and
# other file ops, on many systems, without changing function names.

CPPFLAGS+=	-DHAVE_NBTOOL_CONFIG_H=1 -D_FILE_OFFSET_BITS=64

.PATH:		${LIBDWARF_DIR}

HOST_CPPFLAGS:=	${CPPFLAGS} ${HOST_CPPFLAGS}
CPPFLAGS:=	# empty

.include "${.CURDIR}/../Makefile.nbincludes"
.include <bsd.hostlib.mk>
