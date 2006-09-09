# $NetBSD: Makefile.inc,v 1.4.4.1 2006/09/09 02:41:26 rpaulo Exp $

# Standard kernel compile flags
CFLAGS += -G 0 -mno-abicalls

# Needed so modules loaded in KSEG2 can make calls to KSEG0
CFLAGS += -mlong-calls
