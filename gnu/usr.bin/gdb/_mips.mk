# $NetBSD: _mips.mk,v 1.2 1997/10/19 04:20:54 gwr Exp $

# From gdb/config/mips/nbsd.mh
NM_FILE= config/mips/nm-nbsd.h
NDEP_FILES= mips-nat.c

# From gdb/config/mips/nbsd.mt
TM_FILE= config/mips/tm-nbsd.h
TDEP_FILES= mips-tdep.c solib.c
