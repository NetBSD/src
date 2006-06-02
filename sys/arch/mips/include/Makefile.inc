# $NetBSD: Makefile.inc,v 1.6 2006/06/02 19:45:56 mrg Exp $

# Standard kernel compile flags
CFLAGS += -G 0 -mno-abicalls

# Needed so modules loaded in KSEG2 can make calls to KSEG0
CFLAGS += -mlong-calls
