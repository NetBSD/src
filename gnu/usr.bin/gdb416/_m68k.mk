# $NetBSD: _m68k.mk,v 1.2 1997/09/29 15:37:23 gwr Exp $

# From gdb/config/m68k/nbsd.mh
NM_FILE= config/m68k/nm-nbsd.h
NDEP_FILES= m68knbsd-nat.c

# From gdb/config/m68k/nbsd.mt
TM_FILE= config/m68k/tm-nbsd.h
TDEP_FILES= m68k-tdep.c solib.c

