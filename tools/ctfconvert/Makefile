#	$NetBSD: Makefile,v 1.11.2.1 2025/08/02 05:58:22 perseant Exp $

.include <bsd.hostinit.mk>

.include "${.CURDIR}/../Makefile.ctftools"

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

.include "${.CURDIR}/../Makefile.host"
