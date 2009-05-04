# $NetBSD: Makefile.inc,v 1.1.4.2 2009/05/04 08:10:49 yamt Exp $

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

# NetBSD/atari specific replacements: dev.c

# machine dependant routines
SRCS+=   consio.S dev.c diskio.c
