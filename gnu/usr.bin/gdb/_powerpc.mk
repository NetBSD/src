# $NetBSD: _powerpc.mk,v 1.2 1997/10/19 04:31:58 gwr Exp $

# From gdb/config/powerpc/nbsd.mh
NM_FILE= config/powerpc/nm-nbsd.h
NDEP_FILES= rs6000-nat.c

# From gdb/config/powerpc/nbsd.mt
TM_FILE= config/powerpc/tm-nbsd.h
TDEP_FILES= rs6000-tdep.c solib.c

# Override this for now, to ommit: kcore-nbsd.c
NDEP_CMN= infptrace.c inftarg.c fork-child.c corelow.c
