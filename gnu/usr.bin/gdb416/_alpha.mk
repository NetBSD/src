# $NetBSD: _alpha.mk,v 1.1 1997/09/26 21:48:16 gwr Exp $

# From gdb/config/alpha/nbsd.mh
NM_FILE= config/alpha/nm-nbsd.h
NDEP_FILES= alphanbsd-nat.o

# From gdb/config/alpha/nbsd.mt
TM_FILE= config/alpha/tm-nbsd.h
TDEP_FILES= alpha-tdep.o solib.o

