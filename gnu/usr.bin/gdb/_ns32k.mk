# $NetBSD: _ns32k.mk,v 1.1.1.1 1997/09/26 04:37:02 gwr Exp $

# gdb/config/ns32k/nbsd.mh
NM_FILE= config/ns32k/nm-nbsd.h
NDEP_FILES= ns32knbsd-nat.o

# gdb/config/ns32k/nbsd.mt
TM_FILE= config/ns32k/tm-nbsd.h
TDEP_FILES= ns32k-tdep.o solib.o

