# $NetBSD: Makefile.inc,v 1.2 2003/07/26 18:33:52 mrg Exp $

# Standard kernel compile flags
CFLAGS += -G 0 -mno-abicalls
.if ${USE_TOOLS_TOOLCHAIN} == "yes"
CFLAGS += -mno-half-pic
.endif

# Needed so modules loaded in KSEG2 can make calls to KSEG0
CFLAGS += -mlong-calls
