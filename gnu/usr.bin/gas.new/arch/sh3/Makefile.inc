#	$NetBSD: Makefile.inc,v 1.3 2001/02/23 22:46:37 msaitoh Exp $

TARG_CPU_C=	tc-sh.c
.if ${OBJECT_FMT} == "ELF"
# ELF part
TARG_CPU_C+=	e-shelf.c
OBJ_FORMAT_C=	obj-elf.c
CPPFLAGS+=-DDEFAULT_ELF
#   Default emul depends on endian-ness
.if	(${MACHINE_ARCH} == "sh3eb")
CPPFLAGS+=-DDEFAULT_EMULATION=\"shbelf\"
.else
CPPFLAGS+=-DDEFAULT_EMULATION=\"shlelf\"
.endif
.else
# COFF part
OBJ_FORMAT_C=	obj-coff.c
#   Default byte ordef
.if	(${MACHINE_ARCH} == "sh3el")
CPPFLAGS+=-DTARGET_LITTLE_ENDIAN_DEFAULT
.endif
.endif

ATOF_TARG_C=	atof-ieee.c
