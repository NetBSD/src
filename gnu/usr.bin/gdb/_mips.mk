# $NetBSD: _mips.mk,v 1.3 1997/10/19 04:31:57 gwr Exp $

# From gdb/config/mips/nbsd.mh
NM_FILE= config/mips/nm-nbsd.h
NDEP_FILES= mips-nat.c

# From gdb/config/mips/nbsd.mt
TM_FILE= config/mips/tm-nbsd.h
TDEP_FILES= mips-tdep.c solib.c

# Override this for now, to ommit: kcore-nbsd.c
NDEP_CMN= infptrace.c inftarg.c fork-child.c corelow.c
