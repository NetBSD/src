# $NetBSD: _vax.mk,v 1.1.1.1 1997/09/26 04:37:02 gwr Exp $

# From gdb/config/vax/nbsd.mh
NM_FILE= config/vax/nm-nbsd.h
NDEP_FILES= vaxnbsd-nat.o

# From gdb/config/vax/nbsd.mt
TM_FILE= config/vax/tm-nbsd.h
TDEP_FILES= vax-tdep.o solib.o

