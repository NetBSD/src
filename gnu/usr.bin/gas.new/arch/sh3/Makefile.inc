#	$NetBSD: Makefile.inc,v 1.1.4.1 1999/12/27 18:29:18 wrstuden Exp $

TARG_CPU_C=	tc-sh.c
.if ${OBJECT_FMT} == "ELF"
OBJ_FORMAT_C=	obj-elf.c
CPPFLAGS+=-DDEFAULT_ELF
.else
OBJ_FORMAT_C=	obj-coff.c
.endif
ATOF_TARG_C=	atof-ieee.c
