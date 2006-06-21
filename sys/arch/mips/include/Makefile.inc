# $NetBSD: Makefile.inc,v 1.3.16.1 2006/06/21 14:53:38 yamt Exp $

# Standard kernel compile flags
CFLAGS += -G 0 -mno-abicalls

# Needed so modules loaded in KSEG2 can make calls to KSEG0
CFLAGS += -mlong-calls
