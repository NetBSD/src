#	$NetBSD: Makefile.inc,v 1.1 1998/05/19 19:09:32 tv Exp $

TARG_CPU_C=	tc-arm.c
OBJ_FORMAT_C=	obj-aout.c
ATOF_TARG_C=	atof-ieee.c

CPPFLAGS+=-DBFD_ASSEMBLER=1

TARGET_CPU=arm
