# $NetBSD: Makefile.inc,v 1.4.12.1 2006/05/24 15:48:13 tron Exp $

# Standard kernel compile flags
CFLAGS += -G 0 -mno-abicalls
.if ${HAVE_GCC} == "2"
CFLAGS += -mno-half-pic
.endif

# Needed so modules loaded in KSEG2 can make calls to KSEG0
CFLAGS += -mlong-calls
