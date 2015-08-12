# $NetBSD: Makefile.inc,v 1.3 2015/08/12 18:32:48 tsutsui Exp $

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

# machine dependent routines
SRCS+=   consio.S diskio.c
