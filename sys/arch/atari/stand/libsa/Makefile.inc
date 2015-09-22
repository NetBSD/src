# $NetBSD: Makefile.inc,v 1.2.30.1 2015/09/22 12:05:38 skrll Exp $

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

# machine dependent routines
SRCS+=   consio.S diskio.c
