#	$NetBSD: Makefile.inc,v 1.3 1999/01/11 10:55:21 christos Exp $

TARG_CPU_C=	tc-i386.c
.if ${OBJECT_FORMAT} == "ELF"
OBJ_FORMAT_C=	obj-elf.c
CPPFLAGS+=-DDEFAULT_ELF
.else
OBJ_FORMAT_C=	obj-aout.c
.endif
ATOF_TARG_C=	atof-ieee.c

CPPFLAGS+=-DBFD_ASSEMBLER=1
