#	$NetBSD: Makefile.inc,v 1.4 1999/01/11 11:49:20 christos Exp $

TARG_CPU_C=	tc-i386.c
.if (${OBJECT_FMT} == "ELF")
OBJ_FORMAT_C=	obj-elf.c
CPPFLAGS+=-DDEFAULT_ELF
.else
OBJ_FORMAT_C=	obj-aout.c
.endif
ATOF_TARG_C=	atof-ieee.c

CPPFLAGS+=-DBFD_ASSEMBLER=1
