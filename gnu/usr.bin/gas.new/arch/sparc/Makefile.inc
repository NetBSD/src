#	$NetBSD: Makefile.inc,v 1.2 1999/01/31 21:17:27 christos Exp $

TARG_CPU_C=	tc-sparc.c
.if (${OBJECT_FMT} == "ELF")
OBJ_FORMAT_C=	obj-elf.c
CPPFLAGS+=-DDEFAULT_ELF
.else
OBJ_FORMAT_C=	obj-aout.c
.endif
ATOF_TARG_C=	atof-ieee.c

CPPFLAGS+=-DBFD_ASSEMBLER=1
