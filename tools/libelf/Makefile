#	$NetBSD: Makefile,v 1.2 2009/12/23 20:28:29 cegger Exp $

HOSTLIB=	elf

SRCS=		elf_begin.c						\
		elf_cntl.c						\
		elf_end.c elf_errmsg.c elf_errno.c			\
		elf_data.c						\
		elf_fill.c elf_flag.c					\
		elf_getarhdr.c elf_getarsym.c elf_getbase.c		\
		    elf_getident.c					\
		elf_hash.c						\
		elf_kind.c						\
		elf_memory.c						\
		elf_next.c						\
		elf_rand.c elf_rawfile.c				\
		elf_phnum.c						\
		elf_shnum.c elf_shstrndx.c elf_scn.c elf_strptr.c	\
		elf_update.c						\
		elf_version.c						\
		gelf_cap.c						\
		gelf_checksum.c						\
		gelf_dyn.c						\
		gelf_ehdr.c						\
		gelf_getclass.c						\
		gelf_fsize.c						\
		gelf_move.c						\
		gelf_phdr.c						\
		gelf_rel.c gelf_rela.c					\
		gelf_shdr.c gelf_sym.c gelf_syminfo.c gelf_symshndx.c	\
		gelf_xlate.c						\
		libelf.c						\
		libelf_align.c libelf_allocate.c libelf_ar.c		\
		libelf_checksum.c					\
		libelf_data.c						\
		libelf_ehdr.c libelf_extended.c				\
		libelf_phdr.c						\
		libelf_shdr.c						\
		libelf_xlate.c						\
		${LIBELF_GENSRCS}

LIBELF_GENSRCS=	libelf_fsize.c libelf_msize.c libelf_convert.c

LIBELF_DIR=	${.CURDIR}/../../external/bsd/libelf/dist

CLEANFILES+=	${LIBELF_GENSRCS}
CPPFLAGS+=	-I${.CURDIR}/../compat -I${LIBELF_DIR}
CPPFLAGS+=	-I${TOOLDIR}/include/nbinclude

CPPFLAGS+=	-DLIBELF_TEST_HOOKS

libelf_convert.c:	elf_types.m4 libelf_convert.m4
libelf_fsize.c:		elf_types.m4 libelf_fsize.m4
libelf_msize.c:		elf_types.m4 libelf_msize.m4

BUILD_OSTYPE!=  uname -s

# Disable use of pre-compiled headers on Darwin.
.if ${BUILD_OSTYPE} == "Darwin"
CPPFLAGS+=	-no-cpp-precomp
.endif

# -D_FILE_OFFSET_BITS=64 produces a much more amenable `struct stat', and
# other file ops, on many systems, without changing function names.

CPPFLAGS+=	-DHAVE_NBTOOL_CONFIG_H=1 -D_FILE_OFFSET_BITS=64

.PATH:		${LIBELF_DIR}

HOST_CPPFLAGS:=	${CPPFLAGS}
CPPFLAGS:=	# empty

.include "${.CURDIR}/../Makefile.disklabel"
.include <bsd.hostlib.mk>

# Keep the .SUFFIXES line after the include of bsd.hostlib.mk
M4OBJDIR!=	cd ${.CURDIR}/../m4 && ${PRINTOBJDIR}
.SUFFIXES:	.m4 .c
.m4.c:
	${M4OBJDIR}/m4 -D SRCDIR=${LIBELF_DIR} ${.IMPSRC} > ${.TARGET}
