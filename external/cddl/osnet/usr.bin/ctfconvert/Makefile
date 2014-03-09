#	$NetBSD: Makefile,v 1.5 2014/03/09 17:05:23 christos Exp $

# $FreeBSD: src/cddl/usr.bin/ctfconvert/Makefile,v 1.1.4.1 2009/08/03 08:13:06 kensmith Exp $

.include <bsd.own.mk>

.include	"../../Makefile.inc"
.include	"../../Makefile.ctf"

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


.include <bsd.prog.mk>
