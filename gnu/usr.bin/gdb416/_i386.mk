# $NetBSD: _i386.mk,v 1.1.1.1 1997/09/26 04:37:02 gwr Exp $

# From gdb/config/i386/nbsd.mh
NM_FILE= config/i386/nm-nbsd.h
NDEP_FILES= i386nbsd-nat.o

# From gdb/config/i386/nbsd.mt
TM_FILE= config/i386/tm-nbsd.h
TDEP_FILES= i386-tdep.o i387-tdep.o solib.o

