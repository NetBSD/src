# $NetBSD: _arm32.mk,v 1.3 1998/05/22 17:18:02 tv Exp $

# From gdb/config/arm/arm.mh
NM_FILE= config/arm/nm-nbsd.h
NDEP_FILES= armb-nat.c arm-convert.s

# From gdb/config/arm/arm.mt
TM_FILE= config/arm/tm-nbsd.h
TDEP_FILES= arm-tdep.c solib.c

# Override this for now, to ommit: kcore-nbsd.c
NDEP_CMN= infptrace.c inftarg.c fork-child.c corelow.c
