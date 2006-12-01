# $NetBSD: Makefile.inc,v 1.2 2006/12/01 13:52:46 he Exp $

CPPFLAGS+= -D__daddr_t=int32_t
CFLAGS+= -Os
