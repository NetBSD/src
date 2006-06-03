# $NetBSD: Makefile.inc,v 1.4.6.2 2006/06/03 19:44:03 kardel Exp $

# Standard kernel compile flags
CFLAGS += -G 0 -mno-abicalls

# Needed so modules loaded in KSEG2 can make calls to KSEG0
CFLAGS += -mlong-calls
