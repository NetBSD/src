# $NetBSD: Makefile.inc,v 1.2.12.1 2017/12/03 11:35:58 jdolecek Exp $

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

# machine dependent routines
SRCS+=   consio.S diskio.c
