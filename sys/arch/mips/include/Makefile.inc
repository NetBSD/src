# $NetBSD: Makefile.inc,v 1.4 2005/12/11 12:18:09 christos Exp $

# Standard kernel compile flags
CFLAGS += -G 0 -mno-abicalls
.if ${HAVE_GCC3} == "no"
CFLAGS += -mno-half-pic
.endif

# Needed so modules loaded in KSEG2 can make calls to KSEG0
CFLAGS += -mlong-calls
