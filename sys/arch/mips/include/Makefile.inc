# $NetBSD: Makefile.inc,v 1.1.24.1 2004/08/03 10:37:38 skrll Exp $

# Standard kernel compile flags
CFLAGS += -G 0 -mno-abicalls
.if ${HAVE_GCC3} == "no"
CFLAGS += -mno-half-pic
.endif

# Needed so modules loaded in KSEG2 can make calls to KSEG0
CFLAGS += -mlong-calls
