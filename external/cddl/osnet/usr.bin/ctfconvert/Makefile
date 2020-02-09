#	$NetBSD: Makefile,v 1.9 2020/02/09 07:55:13 fox Exp $

# $FreeBSD: head/cddl/usr.bin/ctfconvert/Makefile 314654 2017-03-04 11:30:04Z ngie $

.include <bsd.own.mk>

.include	"../../Makefile.inc"
.include	"../../Makefile.ctf"

.PATH:          ${OPENSOLARIS_USR_DISTDIR}/tools/ctf/cvt

DEBUG_FLAGS=	-g

PROG=		ctfconvert

SRCS=		alist.c \
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

COPTS.dwarf.c +=	${GCC_NO_STRINGOP_TRUNCATION}
COPTS.st_parse.c +=	-Wno-format-nonliteral
COPTS.util.c +=		-Wno-format-nonliteral

.include <bsd.prog.mk>
