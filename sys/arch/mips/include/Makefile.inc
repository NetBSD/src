# $NetBSD: Makefile.inc,v 1.1.8.2 2002/01/10 19:45:59 thorpej Exp $

# Standard kernel compile flags
CFLAGS += -G 0 -mno-abicalls -mno-half-pic

# Needed so modules loaded in KSEG2 can make calls to KSEG0
CFLAGS += -mlong-calls
