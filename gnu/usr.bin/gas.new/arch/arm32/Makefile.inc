#	$NetBSD: Makefile.inc,v 1.2 1998/05/22 17:19:12 tv Exp $

TARG_CPU_C=	tc-arm.c
OBJ_FORMAT_C=	obj-aout.c
ATOF_TARG_C=	atof-ieee.c

CPPFLAGS+=-DBFD_ASSEMBLER=1
