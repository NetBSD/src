#	$NetBSD: Makefile.inc,v 1.1 1999/04/30 14:53:53 kleink Exp $

TARG_CPU_C=	tc-m68k.c m68k-parse.c
.if (${OBJECT_FMT} == "ELF") || defined(BOOTSTRAP_ELF)
OBJ_FORMAT_C=	obj-elf.c
CPPFLAGS+=-DDEFAULT_ELF
.else
OBJ_FORMAT_C=	obj-aout.c
.endif
ATOF_TARG_C=	atof-ieee.c

CLEANFILES+=	m68k-parse.c
