# $NetBSD: _m68k.mk,v 1.1.1.1 1997/09/26 04:37:02 gwr Exp $

# From gdb/config/m68k/nbsd.mh
NM_FILE= config/m68k/nm-nbsd.h
NDEP_FILES= m68knbsd-nat.o

# From gdb/config/m68k/nbsd.mt
TM_FILE= config/m68k/tm-nbsd.h
TDEP_FILES= m68k-tdep.o solib.o

