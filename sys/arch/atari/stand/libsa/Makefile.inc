# $NetBSD: Makefile.inc,v 1.1 2009/03/31 11:42:07 tsutsui Exp $

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

# NetBSD/atari specific replacements: dev.c

# machine dependant routines
SRCS+=   consio.S dev.c diskio.c
