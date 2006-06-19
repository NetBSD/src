# $NetBSD: Makefile.inc,v 1.5.2.1 2006/06/19 03:44:52 chap Exp $

# Standard kernel compile flags
CFLAGS += -G 0 -mno-abicalls

# Needed so modules loaded in KSEG2 can make calls to KSEG0
CFLAGS += -mlong-calls
