# $NetBSD: Makefile.inc,v 1.2 2011/06/30 20:09:21 wiz Exp $

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

# NetBSD/atari specific replacements: dev.c

# machine dependent routines
SRCS+=   consio.S dev.c diskio.c
