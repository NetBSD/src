#	$NetBSD: Makefile.inc,v 1.6 1999/03/17 00:46:35 thorpej Exp $

TARG_CPU_C=	tc-i386.c
.if (${OBJECT_FMT} == "ELF") || defined(BOOTSTRAP_ELF)
OBJ_FORMAT_C=	obj-elf.c
CPPFLAGS+=-DDEFAULT_ELF
.else
OBJ_FORMAT_C=	obj-aout.c
.endif
ATOF_TARG_C=	atof-ieee.c
