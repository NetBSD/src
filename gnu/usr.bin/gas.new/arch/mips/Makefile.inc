#	$NetBSD: Makefile.inc,v 1.3 1999/02/04 23:24:51 tv Exp $

.include <bsd.own.mk>		# MACHINE_GNU_ARCH

TARG_CPU_C=	tc-mips.c itbl-ops.c itbl-parse.c itbl-lex.c e-mipself.c
OBJ_FORMAT_C=	obj-elf.c
ATOF_TARG_C=	atof-ieee.c

CPPFLAGS+= -I.
SRCS+=	itbl-parse.h
YHEADER=1

#   Default emul depends on endian-ness
.if	(${MACHINE_GNU_ARCH} == "mipsel")
CPPFLAGS+=-DDEFAULT_EMULATION=\"mipslelf\"
.else
CPPFLAGS+=-DDEFAULT_EMULATION=\"mipsbelf\"
.endif
