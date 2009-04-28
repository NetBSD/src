# $NetBSD: Makefile.inc,v 1.1.2.2 2009/04/28 07:33:54 skrll Exp $

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

# NetBSD/atari specific replacements: dev.c

# machine dependant routines
SRCS+=   consio.S dev.c diskio.c
