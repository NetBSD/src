#	$NetBSD: Makefile.inc,v 1.1 1998/08/15 18:34:38 kristerw Exp $

TARG_CPU_C=	tc-sparc.c
OBJ_FORMAT_C=	obj-aout.c
ATOF_TARG_C=	atof-ieee.c

CPPFLAGS+=-DBFD_ASSEMBLER=1
