#	$NetBSD: Makefile.inc,v 1.1 1998/08/27 16:09:20 tv Exp $

TARG_CPU_C=	tc-alpha.c
OBJ_FORMAT_C=	obj-elf.c
ATOF_TARG_C=	atof-ieee.c

CPPFLAGS+=-DBFD_ASSEMBLER=1
