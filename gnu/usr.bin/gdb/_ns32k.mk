# $NetBSD: _ns32k.mk,v 1.2 1997/09/29 15:37:24 gwr Exp $

# gdb/config/ns32k/nbsd.mh
NM_FILE= config/ns32k/nm-nbsd.h
NDEP_FILES= ns32knbsd-nat.c

# gdb/config/ns32k/nbsd.mt
TM_FILE= config/ns32k/tm-nbsd.h
TDEP_FILES= ns32k-tdep.c solib.c

