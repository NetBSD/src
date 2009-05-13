# $NetBSD: Makefile.inc,v 1.1.6.2 2009/05/13 17:16:32 jym Exp $

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

# NetBSD/atari specific replacements: dev.c

# machine dependant routines
SRCS+=   consio.S dev.c diskio.c
