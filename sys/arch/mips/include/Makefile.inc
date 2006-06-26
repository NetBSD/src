# $NetBSD: Makefile.inc,v 1.4.8.2 2006/06/26 12:45:13 yamt Exp $

# Standard kernel compile flags
CFLAGS += -G 0 -mno-abicalls

# Needed so modules loaded in KSEG2 can make calls to KSEG0
CFLAGS += -mlong-calls
