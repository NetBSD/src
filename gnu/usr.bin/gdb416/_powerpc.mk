# $NetBSD: _powerpc.mk,v 1.1 1997/10/17 21:30:18 gwr Exp $

# From gdb/config/powerpc/nbsd.mh
NM_FILE= config/powerpc/nm-nbsd.h
NDEP_FILES= rs6000-nat.c

# From gdb/config/powerpc/nbsd.mt
TM_FILE= config/powerpc/tm-nbsd.h
TDEP_FILES= rs6000-tdep.c solib.c
