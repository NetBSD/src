#	$NetBSD: Makefile.inc,v 1.2 1998/06/26 20:47:08 tv Exp $

TARG_CPU_C=	tc-i386.c
OBJ_FORMAT_C=	obj-aout.c
ATOF_TARG_C=	atof-ieee.c

CPPFLAGS+=-DBFD_ASSEMBLER=1
