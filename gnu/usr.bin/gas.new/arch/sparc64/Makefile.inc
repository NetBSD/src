#	$NetBSD: Makefile.inc,v 1.1 1998/11/23 09:40:27 mrg Exp $

TARG_CPU_C=	tc-sparc.c
OBJ_FORMAT_C=	obj-elf.c
ATOF_TARG_C=	atof-ieee.c

CPPFLAGS+=-DBFD_ASSEMBLER=1
