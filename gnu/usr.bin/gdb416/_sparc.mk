# $NetBSD: _sparc.mk,v 1.2 1997/09/29 15:37:25 gwr Exp $

# From gdb/config/sparc/nbsd.mh
NM_FILE= config/sparc/nm-nbsd.h
NDEP_FILES= sparcnbsd-nat.c

# From gdb/config/sparc/nbsd.mt
TM_FILE= config/sparc/tm-nbsd.h
TDEP_FILES= sparc-tdep.c solib.c

