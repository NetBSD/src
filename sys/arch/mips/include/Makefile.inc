# $NetBSD: Makefile.inc,v 1.1 2001/10/05 15:36:46 simonb Exp $

# Standard kernel compile flags
CFLAGS += -G 0 -mno-abicalls -mno-half-pic

# Needed so modules loaded in KSEG2 can make calls to KSEG0
CFLAGS += -mlong-calls
