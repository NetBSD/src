#	$NetBSD: Makefile.inc,v 1.1 1998/10/08 14:17:18 matthias Exp $

TARG_CPU_C=	tc-ns32k.c
OBJ_FORMAT_C=	obj-aout.c
ATOF_TARG_C=	atof-ieee.c

CPPFLAGS+=-DBFD_ASSEMBLER=1 -Uns32k
