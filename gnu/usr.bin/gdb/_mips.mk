# $NetBSD: _mips.mk,v 1.4 1997/10/19 20:19:07 jonathan Exp $

# From gdb/config/mips/nbsd.mh
NM_FILE= config/mips/nm-nbsd.h
NDEP_FILES= mipsnbsd-nat.c

# From gdb/config/mips/nbsd.mt
TM_FILE= config/mips/tm-nbsd.h
TDEP_FILES= mips-tdep.c solib.c
