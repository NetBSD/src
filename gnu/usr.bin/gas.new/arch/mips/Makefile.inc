#	$NetBSD: Makefile.inc,v 1.4 1999/03/15 20:34:28 drochner Exp $

.include <bsd.own.mk>		# MACHINE_GNU_ARCH

TARG_CPU_C=	tc-mips.c itbl-ops.c itbl-parse.y itbl-lex.l e-mipself.c
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
