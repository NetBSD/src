# $NetBSD: _mipsel.mk,v 1.1 1998/07/27 02:35:13 tv Exp $

# From gdb/config/mips/nbsd.mh
NM_FILE= config/mips/nm-nbsd.h
NDEP_FILES= mipsnbsd-nat.c

# From gdb/config/mips/nbsd.mt
TM_FILE= config/mips/tm-nbsd.h
TDEP_FILES= mips-tdep.c solib.c
