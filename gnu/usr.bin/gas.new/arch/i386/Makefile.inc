#	$NetBSD: Makefile.inc,v 1.1 1998/05/21 18:46:42 cgd Exp $

TARG_CPU_C=	tc-i386.c
OBJ_FORMAT_C=	obj-aout.c
ATOF_TARG_C=	atof-ieee.c

CPPFLAGS+=-DBFD_ASSEMBLER=1

TARGET_CPU=i386
