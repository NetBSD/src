# $NetBSD: _i386.mk,v 1.2 1997/09/29 15:37:23 gwr Exp $

# From gdb/config/i386/nbsd.mh
NM_FILE= config/i386/nm-nbsd.h
NDEP_FILES= i386nbsd-nat.c

# From gdb/config/i386/nbsd.mt
TM_FILE= config/i386/tm-nbsd.h
TDEP_FILES= i386-tdep.c i387-tdep.c solib.c

