#	$NetBSD: Makefile.inc,v 1.2.2.2 1998/11/07 00:56:43 cgd Exp $

.include <bsd.own.mk>		# MACHINE_GNU_ARCH

TARG_CPU_C=	tc-mips.c itbl-ops.c itbl-parse.c itbl-lex.c e-mipself.c
OBJ_FORMAT_C=	obj-elf.c
ATOF_TARG_C=	atof-ieee.c

SRCS+=	itbl-parse.h
YHEADER=1

CPPFLAGS+=-DBFD_ASSEMBLER=1 

#    mips-elf targets need EMUL support 
CPPFLAGS+= -DCROSS_COMPILE=1 -DUSE_EMULATIONS=1 -I.
CPPFLAGS+=-DEMULATIONS="&mipslelf, &mipsbelf, &mipself,"

#   Default emul depends on endian-ness
.if	(${MACHINE_GNU_ARCH} == "mipsel")
CPPFLAGS+=-DDEFAULT_EMULATION=\"mipslelf\"
.else
CPPFLAGS+=-DDEFAULT_EMULATION=\"mipsbelf\"
.endif
