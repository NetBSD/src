#	$NetBSD: Makefile.inc,v 1.1 1998/09/28 21:11:21 jonathan Exp $

TARG_CPU_C=	tc-mips.c itbl-ops.c itbl-parse.c itbl-lex.c
OBJ_FORMAT_C=	obj-elf.c
ATOF_TARG_C=	atof-ieee.c

CPPFLAGS+=-DBFD_ASSEMBLER=1 -I.
SRCS+=	itbl-parse.h

YHEADER=1
