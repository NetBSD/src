# $NetBSD: _mips.mk,v 1.1 1997/10/17 21:30:07 gwr Exp $

# From gdb/config/mips/decstation.mh
NM_FILE= config/mips/nm-mips.h
NDEP_FILES= mips-nat.c

# From gdb/config/mips/decstation.mt
TM_FILE= config/mips/tm-mips.h
TDEP_FILES= mips-tdep.c solib.c
