#	$NetBSD: Makefile.inc,v 1.3 1999/02/04 23:25:08 tv Exp $

TARG_CPU_C=	tc-sparc.c
.if (${OBJECT_FMT} == "ELF")
OBJ_FORMAT_C=	obj-elf.c
CPPFLAGS+=-DDEFAULT_ELF
.else
OBJ_FORMAT_C=	obj-aout.c
.endif
ATOF_TARG_C=	atof-ieee.c
