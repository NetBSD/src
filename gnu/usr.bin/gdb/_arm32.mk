# $NetBSD: _arm32.mk,v 1.2 1997/10/19 04:31:55 gwr Exp $

# From gdb/config/arm/arm.mh
NM_FILE= config/nm-nbsd.h
NDEP_FILES= arm-xdep.c arm-convert.s

# From gdb/config/arm/arm.mt
TM_FILE= config/arm/tm-arm.h
TDEP_FILES= arm-tdep.c solib.c

# Override this for now, to ommit: kcore-nbsd.c
NDEP_CMN= infptrace.c inftarg.c fork-child.c corelow.c
