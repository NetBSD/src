# $NetBSD: Makefile.inc,v 1.4.6.1 2006/04/22 11:37:42 simonb Exp $

# Standard kernel compile flags
CFLAGS += -G 0 -mno-abicalls
.if ${HAVE_GCC} == "2"
CFLAGS += -mno-half-pic
.endif

# Needed so modules loaded in KSEG2 can make calls to KSEG0
CFLAGS += -mlong-calls
