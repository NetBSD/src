# $NetBSD: _arm32.mk,v 1.1 1997/10/17 21:30:12 gwr Exp $

# From gdb/config/arm/arm.mh
NM_FILE= config/nm-nbsd.h
NDEP_FILES= arm-xdep.c arm-convert.s

# From gdb/config/arm/arm.mt
TM_FILE= config/arm/tm-arm.h
TDEP_FILES= arm-tdep.c solib.c
