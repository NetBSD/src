# $NetBSD: _sparc.mk,v 1.1.1.1 1997/09/26 04:37:02 gwr Exp $

# From gdb/config/sparc/nbsd.mh
NM_FILE= config/sparc/nm-nbsd.h
NDEP_FILES= sparcnbsd-nat.o

# From gdb/config/sparc/nbsd.mt
TM_FILE= config/sparc/tm-nbsd.h
TDEP_FILES= sparc-tdep.o solib.o

